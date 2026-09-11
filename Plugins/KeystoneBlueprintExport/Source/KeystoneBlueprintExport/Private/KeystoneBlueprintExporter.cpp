#include "KeystoneBlueprintExporter.h"

#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"          // UEdGraphNode_Comment (module: UnrealEd)
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// The exporter reads the *live* editor model (UEdGraph/UEdGraphNode/UEdGraphPin) directly
// rather than parsing the T3D copy/paste text — same full fidelity, but structured and far
// less brittle across engine versions. Every value below maps 1:1 onto a field in the
// `.bpgraph.json` schema (packages/types/src/blueprint-graph.ts).

namespace
{
    using FJsonWriterRef = TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>>;

    FString GuidStr(const FGuid& G)
    {
        return G.ToString(EGuidFormats::Digits).ToLower();
    }

    /** A pin's category + subtype flattened to a stable string, e.g. `exec`, `bool`,
     *  `object:/Script/Engine.Actor`, `int[]`. Enough to diff a wire's type meaningfully. */
    FString PinTypeString(const FEdGraphPinType& T)
    {
        FString S = T.PinCategory.ToString();
        if (T.PinSubCategoryObject.IsValid())
        {
            S += TEXT(":") + T.PinSubCategoryObject->GetPathName();
        }
        else if (!T.PinSubCategory.IsNone())
        {
            S += TEXT(":") + T.PinSubCategory.ToString();
        }
        switch (T.ContainerType)
        {
            case EPinContainerType::Array: S += TEXT("[]"); break;
            case EPinContainerType::Set:   S += TEXT("{}"); break;
            case EPinContainerType::Map:   S += TEXT("{k:v}"); break;
            default: break;
        }
        return S;
    }

    /** The literal on an unconnected input pin (a diffable value), or empty when the pin is
     *  wired (its value comes from the wire, not a literal). */
    FString PinDefault(const UEdGraphPin* P)
    {
        if (P->LinkedTo.Num() > 0) return FString();
        if (P->DefaultObject) return P->DefaultObject->GetPathName();
        if (!P->DefaultTextValue.IsEmpty()) return P->DefaultTextValue.ToString();
        return P->DefaultValue;
    }

    // A pin's raw PinId GUID is NOT stable across editor loads — some nodes (e.g. math compare
    // nodes' advanced `ErrorTolerance` pin) reconstruct pins on load and regenerate the GUID,
    // which made re-exports churn. So identity is derived instead from the owning node's stable
    // NodeGuid + the pin's direction + name (with an occurrence index when a node has duplicate
    // names). Deterministic across loads, and it makes wire matching semantic rather than tied to
    // a volatile GUID. `Seen` tracks per-(node,dir,name) counts across the whole graph.
    FString StablePinId(UEdGraphNode* N, const UEdGraphPin* P, TMap<FString, int32>& Seen)
    {
        const FString Base = GuidStr(N->NodeGuid) + TEXT(":") +
            (P->Direction == EGPD_Output ? TEXT("o:") : TEXT("i:")) + P->PinName.ToString();
        int32& Count = Seen.FindOrAdd(Base);
        const FString Id = Count == 0 ? Base : FString::Printf(TEXT("%s#%d"), *Base, Count);
        ++Count;
        return Id;
    }

    void WritePin(const FJsonWriterRef& W, const UEdGraphPin* P, const TMap<const UEdGraphPin*, FString>& PinIds)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("id"), PinIds[P]);
        W->WriteValue(TEXT("name"), P->PinName.ToString());
        W->WriteValue(TEXT("direction"), P->Direction == EGPD_Output ? TEXT("output") : TEXT("input"));
        W->WriteValue(TEXT("type"), PinTypeString(P->PinType));
        const FString Def = PinDefault(P);
        if (Def.IsEmpty()) { W->WriteNull(TEXT("defaultValue")); }
        else { W->WriteValue(TEXT("defaultValue"), Def); }

        // Wire endpoints: the stable id of every pin this one connects to (links to pins outside
        // this graph, or on skipped nodes, are dropped). Sorted for determinism.
        TArray<FString> Links;
        Links.Reserve(P->LinkedTo.Num());
        for (const UEdGraphPin* L : P->LinkedTo)
        {
            if (const FString* Id = L ? PinIds.Find(L) : nullptr) Links.Add(*Id);
        }
        Links.Sort();
        W->WriteArrayStart(TEXT("links"));
        for (const FString& L : Links) W->WriteValue(L);
        W->WriteArrayEnd();
        W->WriteObjectEnd();
    }

    void WriteNode(const FJsonWriterRef& W, UEdGraphNode* N, const TMap<const UEdGraphPin*, FString>& PinIds)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("guid"), GuidStr(N->NodeGuid));
        W->WriteValue(TEXT("class"), N->GetClass()->GetName());
        W->WriteValue(TEXT("title"), N->GetNodeTitle(ENodeTitleType::ListView).ToString());
        W->WriteValue(TEXT("x"), N->NodePosX);
        W->WriteValue(TEXT("y"), N->NodePosY);
        if (N->NodeComment.IsEmpty()) { W->WriteNull(TEXT("comment")); }
        else { W->WriteValue(TEXT("comment"), N->NodeComment); }

        // Native pin order (stable across loads, and the order pins appear on the node).
        W->WriteArrayStart(TEXT("pins"));
        for (const UEdGraphPin* P : N->Pins) { if (P) WritePin(W, P, PinIds); }
        W->WriteArrayEnd();
        W->WriteObjectEnd();
    }

    void WriteComment(const FJsonWriterRef& W, UEdGraphNode_Comment* C)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("guid"), GuidStr(C->NodeGuid));
        W->WriteValue(TEXT("text"), C->NodeComment);
        W->WriteValue(TEXT("x"), C->NodePosX);
        W->WriteValue(TEXT("y"), C->NodePosY);
        W->WriteValue(TEXT("width"), C->NodeWidth);
        W->WriteValue(TEXT("height"), C->NodeHeight);
        W->WriteObjectEnd();
    }

    void WriteGraph(const FJsonWriterRef& W, UEdGraph* G, const FString& Type)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("name"), G->GetName());
        W->WriteValue(TEXT("type"), Type);

        // Split comment boxes from real nodes; keep each list deterministically ordered.
        TArray<UEdGraphNode*> Nodes;
        TArray<UEdGraphNode_Comment*> Comments;
        for (UEdGraphNode* N : G->Nodes)
        {
            if (!N) continue;
            if (UEdGraphNode_Comment* C = Cast<UEdGraphNode_Comment>(N)) { Comments.Add(C); }
            else { Nodes.Add(N); }
        }
        Nodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return GuidStr(A.NodeGuid) < GuidStr(B.NodeGuid); });
        Comments.Sort([](const UEdGraphNode_Comment& A, const UEdGraphNode_Comment& B) { return GuidStr(A.NodeGuid) < GuidStr(B.NodeGuid); });

        // Assign every pin in the graph a stable id up front, so both a pin's own id and the
        // link references pointing at it resolve to the same value.
        TMap<const UEdGraphPin*, FString> PinIds;
        TMap<FString, int32> Seen;
        for (UEdGraphNode* N : Nodes)
        {
            for (const UEdGraphPin* P : N->Pins) { if (P) PinIds.Add(P, StablePinId(N, P, Seen)); }
        }

        W->WriteArrayStart(TEXT("nodes"));
        for (UEdGraphNode* N : Nodes) WriteNode(W, N, PinIds);
        W->WriteArrayEnd();

        W->WriteArrayStart(TEXT("comments"));
        for (UEdGraphNode_Comment* C : Comments) WriteComment(W, C);
        W->WriteArrayEnd();

        W->WriteObjectEnd();
    }

    /** Every graph of a Blueprint, tagged with its kind, in a stable order. */
    void GatherGraphs(UBlueprint* BP, TArray<TPair<UEdGraph*, FString>>& Out)
    {
        for (UEdGraph* G : BP->UbergraphPages)        if (G) Out.Add({ G, TEXT("Ubergraph") });
        for (UEdGraph* G : BP->FunctionGraphs)        if (G) Out.Add({ G, TEXT("Function") });
        for (UEdGraph* G : BP->MacroGraphs)           if (G) Out.Add({ G, TEXT("Macro") });
        for (UEdGraph* G : BP->DelegateSignatureGraphs) if (G) Out.Add({ G, TEXT("Delegate") });
        Out.Sort([](const TPair<UEdGraph*, FString>& A, const TPair<UEdGraph*, FString>& B)
        {
            const FString KA = A.Value + TEXT(":") + A.Key->GetName();
            const FString KB = B.Value + TEXT(":") + B.Key->GetName();
            return KA < KB;
        });
    }
}

FString FKeystoneBlueprintExporter::BuildJson(UBlueprint* Blueprint)
{
    FString Out;
    const FJsonWriterRef W = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);

    W->WriteObjectStart();
    W->WriteValue(TEXT("version"), 1);
    W->WriteValue(TEXT("generatedBy"), TEXT("keystone-blueprint-export"));
    W->WriteValue(TEXT("package"), Blueprint->GetOutermost()->GetName()); // /Game/.../BP_Hero
    W->WriteValue(TEXT("asset"), Blueprint->GetName());
    if (Blueprint->ParentClass) { W->WriteValue(TEXT("parentClass"), Blueprint->ParentClass->GetName()); }
    else { W->WriteNull(TEXT("parentClass")); }
    W->WriteValue(TEXT("blueprintType"), StaticEnum<EBlueprintType>()->GetNameStringByValue(Blueprint->BlueprintType));

    TArray<TPair<UEdGraph*, FString>> Graphs;
    GatherGraphs(Blueprint, Graphs);
    W->WriteArrayStart(TEXT("graphs"));
    for (const TPair<UEdGraph*, FString>& GP : Graphs) WriteGraph(W, GP.Key, GP.Value);
    W->WriteArrayEnd();

    W->WriteObjectEnd();
    W->Close();
    return Out;
}

FString FKeystoneBlueprintExporter::ExportFileForPackage(const FString& PackageName)
{
    // /Game/Characters/BP_Hero  ->  <Project>/BlueprintGraphs/Characters/BP_Hero.bpgraph.json
    FString Rel = PackageName;
    if (Rel.StartsWith(TEXT("/Game/"))) { Rel = Rel.RightChop(6); }
    else { Rel = Rel.TrimChar(TEXT('/')); }
    const FString Abs = FPaths::Combine(FPaths::ProjectDir(), ExportSubdir(), Rel + TEXT(".bpgraph.json"));
    return FPaths::ConvertRelativePathToFull(Abs);
}

FString FKeystoneBlueprintExporter::ExportFileFor(UBlueprint* Blueprint)
{
    return ExportFileForPackage(Blueprint->GetOutermost()->GetName());
}

FKeystoneBlueprintExporter::EPackageState FKeystoneBlueprintExporter::ClassifyPackage(const FString& PackageName)
{
    if (PackageName.IsEmpty()) return EPackageState::Unknown;

    // A path under a root that is not mounted (e.g. /ProjectAtlantis/...) cannot hold a package.
    // Checked first so DoesPackageExist is never asked about a root it does not know.
    if (!FPackageName::IsValidLongPackageName(PackageName, /*bIncludeReadOnlyRoots=*/true))
    {
        return EPackageState::Stale;
    }

    // Gone from disk: a completed delete, or a rename's redirector that has since been fixed up.
    if (!FPackageName::DoesPackageExist(PackageName)) return EPackageState::Stale;

    // Still on disk, so ask the registry what is actually in it. On-disk assets only, so an
    // object mid-deletion that is still in memory cannot vouch for a package that is going away.
    IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Assets;
    AR.GetAssetsByPackageName(FName(*PackageName), Assets, /*bIncludeOnlyOnDiskAssets=*/true);

    // The file exists but the registry knows nothing about it: an unmount or an unfinished scan.
    if (Assets.IsEmpty()) return EPackageState::Unknown;

    for (const FAssetData& Asset : Assets)
    {
        if (!Asset.IsRedirector()) return EPackageState::Live;
    }

    // Nothing but an ObjectRedirector: the stub a rename leaves at the old path.
    return EPackageState::Stale;
}

bool FKeystoneBlueprintExporter::DeleteExportForPackage(const FString& PackageName)
{
    if (PackageName.IsEmpty()) return false;

    const FString File = ExportFileForPackage(PackageName);
    if (!IFileManager::Get().FileExists(*File)) return false;

    if (IFileManager::Get().Delete(*File))
    {
        UE_LOG(LogTemp, Log, TEXT("[keystone] pruned stale export for %s"), *PackageName);
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("[keystone] failed to prune %s"), *File);
    return false;
}

int32 FKeystoneBlueprintExporter::PruneOrphanExports(const TSet<FString>& ExpectedFiles)
{
    const FString Root = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), ExportSubdir()));

    TArray<FString> OnDisk;
    IFileManager::Get().FindFilesRecursive(OnDisk, *Root, TEXT("*.bpgraph.json"), /*Files=*/true, /*Directories=*/false);

    int32 Removed = 0;
    for (const FString& Found : OnDisk)
    {
        const FString Full = FPaths::ConvertRelativePathToFull(Found);
        if (ExpectedFiles.Contains(Full)) continue;

        // The path rule is lossy: /Game/X collapses to X, while /Engine/X keeps its Engine/
        // prefix, so a file cannot be mapped back to exactly one package. Try both readings and
        // delete only when every reading is conclusively stale — a live asset or an unknown
        // under either reading keeps the file.
        FString Rel = Full;
        FPaths::MakePathRelativeTo(Rel, *(Root / TEXT("")));
        Rel.RemoveFromEnd(TEXT(".bpgraph.json"));

        if (ClassifyPackage(TEXT("/Game/") + Rel) != EPackageState::Stale) continue;
        if (ClassifyPackage(TEXT("/") + Rel) != EPackageState::Stale) continue;

        if (IFileManager::Get().Delete(*Full))
        {
            UE_LOG(LogTemp, Log, TEXT("[keystone] pruned orphaned export %s"), *Rel);
            Removed++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[keystone] failed to prune %s"), *Full);
        }
    }

    return Removed;
}

bool FKeystoneBlueprintExporter::ExportOne(UBlueprint* Blueprint, FKeystoneExportResult& InOutResult)
{
    if (!Blueprint) { InOutResult.Failed++; return false; }
    InOutResult.Scanned++;

    const FString Json = BuildJson(Blueprint);
    const FString File = ExportFileFor(Blueprint);

    // Skip the write (and the git churn) when nothing changed.
    FString Existing;
    if (FFileHelper::LoadFileToString(Existing, *File) && Existing.Equals(Json, ESearchCase::CaseSensitive))
    {
        InOutResult.Unchanged++;
        return true;
    }
    if (!FFileHelper::SaveStringToFile(Json, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Warning, TEXT("[keystone] failed to write %s"), *File);
        InOutResult.Failed++;
        return false;
    }
    InOutResult.Written++;
    return true;
}

FKeystoneExportResult FKeystoneBlueprintExporter::ExportAll(const FString& RootPath)
{
    FKeystoneExportResult R;

    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // If the editor's startup scan is still running the registry is incomplete: the export would
    // miss Blueprints, and the prune would have to keep every file it cannot account for. This is
    // a manual, one-off action, so blocking until the scan finishes is acceptable.
    if (ARM.Get().IsLoadingAssets())
    {
        ARM.Get().WaitForCompletion();
    }

    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.PackagePaths.Add(*RootPath);
    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;
    ARM.Get().GetAssets(Filter, Assets);

    // Manifest: package -> committed export path, so Keystone can resolve a changed .uasset to
    // its .bpgraph.json without re-deriving the path rule.
    FString ManifestJson;
    const FJsonWriterRef MW = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&ManifestJson);
    MW->WriteObjectStart();
    MW->WriteValue(TEXT("version"), 1);
    MW->WriteValue(TEXT("generatedBy"), TEXT("keystone-blueprint-export"));
    MW->WriteValue(TEXT("project"), FApp::GetProjectName());
    MW->WriteArrayStart(TEXT("entries"));

    // Every export a live Blueprint maps to. Anything else under the folder is an orphan left
    // behind by a rename or a delete, which the sweep prunes below.
    TSet<FString> ExpectedFiles;
    ExpectedFiles.Reserve(Assets.Num());

    Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
    for (const FAssetData& AD : Assets)
    {
        UBlueprint* BP = Cast<UBlueprint>(AD.GetAsset());
        if (!BP) { R.Failed++; continue; }
        ExportOne(BP, R);

        const FString File = ExportFileFor(BP);
        ExpectedFiles.Add(File);

        FString RepoRel = File;
        FPaths::MakePathRelativeTo(RepoRel, *(FPaths::ProjectDir())); // e.g. BlueprintGraphs/Characters/BP_Hero.bpgraph.json
        MW->WriteObjectStart();
        MW->WriteValue(TEXT("package"), BP->GetOutermost()->GetName());
        MW->WriteValue(TEXT("asset"), BP->GetName());
        MW->WriteValue(TEXT("exportPath"), RepoRel);
        MW->WriteObjectEnd();
    }

    MW->WriteArrayEnd();
    MW->WriteObjectEnd();
    MW->Close();

    const FString ManifestFile = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), ExportSubdir(), TEXT("keystone-blueprint-manifest.json")));
    if (FFileHelper::SaveStringToFile(ManifestJson, *ManifestFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        R.ManifestPath = ManifestFile;
    }

    // Catch-all for orphans the live hooks missed — exports written before pruning existed, or
    // renames made while the editor was closed. The sweep is the only place with a complete
    // picture of what *should* be on disk, so it is the only place this can be done safely.
    R.Pruned = PruneOrphanExports(ExpectedFiles);

    UE_LOG(LogTemp, Log, TEXT("[keystone] export: scanned=%d written=%d unchanged=%d failed=%d pruned=%d"),
        R.Scanned, R.Written, R.Unchanged, R.Failed, R.Pruned);
    return R;
}
