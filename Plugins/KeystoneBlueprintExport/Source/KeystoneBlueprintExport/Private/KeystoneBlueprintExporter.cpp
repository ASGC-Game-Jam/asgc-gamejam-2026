#include "KeystoneBlueprintExporter.h"

#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"          // UEdGraphNode_Comment (module: UnrealEd)
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionComment.h"
#include "MaterialGraph/MaterialGraph.h"             // module: UnrealEd
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Comment.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"                      // TPropertyValueIterator
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/UserDefinedEnum.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Engine/DataTable.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Engine/LevelScriptBlueprint.h"
#include "GameFramework/WorldSettings.h"
#include "Components/ActorComponent.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/StructOnScope.h"
#include "UObject/TextProperty.h"
#include "UObject/EnumProperty.h"
#include "UObject/FieldPathProperty.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// The exporter reads the *live* editor model (UEdGraph/UEdGraphNode/UEdGraphPin) directly
// rather than parsing the T3D copy/paste text — same full fidelity, but structured and far
// less brittle across engine versions. Every value below maps 1:1 onto a field in the
// `.bpgraph.json` schema (packages/types/src/blueprint-graph.ts). Blueprints, Materials and
// Niagara assets all end up as UEdGraphs, so one writer serves every asset kind; only how the
// graphs are *found* (and, for materials, how nodes are identified) differs per kind.

namespace
{
    using FJsonWriterRef = TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>>;

    /** Stable identity for a node across exports. Blueprints/Niagara persist NodeGuid; material
     *  graph nodes are rebuilt every time (fresh random NodeGuid), so they map to their expression. */
    using FNodeIdFn = TFunction<FString(const UEdGraphNode*)>;

    struct FGraphEntry
    {
        UEdGraph* Graph = nullptr;
        FString Type;
        FString Name;
    };

    // Niagara is an engine *plugin*; match its classes by path so this module neither links it
    // nor fails to load in a project that has Niagara disabled.
    const FTopLevelAssetPath NiagaraClassPaths[] =
    {
        FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraSystem")),
        FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraEmitter")),
        FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraScript")),
    };

    bool IsNiagaraAsset(const UObject* Asset)
    {
        for (const UClass* C = Asset ? Asset->GetClass() : nullptr; C; C = C->GetSuperClass())
        {
            const FTopLevelAssetPath Path = C->GetClassPathName();
            for (const FTopLevelAssetPath& N : NiagaraClassPaths) { if (Path == N) return true; }
        }
        return false;
    }

    FString GuidStr(const FGuid& G)
    {
        return G.ToString(EGuidFormats::Digits).ToLower();
    }

    FString DefaultNodeId(const UEdGraphNode* N)
    {
        return GuidStr(N->NodeGuid);
    }

    /** A material expression's persisted guid (or, if it somehow has none, one derived from its
     *  stable object path inside the package). */
    FString ExpressionId(const UMaterialExpression* E)
    {
        if (E->MaterialExpressionGuid.IsValid()) return GuidStr(E->MaterialExpressionGuid);
        return GuidStr(FGuid::NewDeterministicGuid(E->GetPathName(E->GetOutermost())));
    }

    FString MaterialNodeId(const UEdGraphNode* N)
    {
        if (const UMaterialGraphNode* MN = Cast<UMaterialGraphNode>(N))
        {
            if (MN->MaterialExpression) return ExpressionId(MN->MaterialExpression);
        }
        else if (const UMaterialGraphNode_Comment* CN = Cast<UMaterialGraphNode_Comment>(N))
        {
            if (CN->MaterialExpressionComment) return ExpressionId(CN->MaterialExpressionComment);
        }
        else if (N->IsA<UMaterialGraphNode_Root>())
        {
            return GuidStr(FGuid(0, 0, 0, 1)); // the single material-output node
        }
        return DefaultNodeId(N);
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
    // id + the pin's direction + name (with an occurrence index when a node has duplicate
    // names). Deterministic across loads, and it makes wire matching semantic rather than tied to
    // a volatile GUID. `Seen` tracks per-(node,dir,name) counts across the whole graph.
    FString StablePinId(const FString& NodeId, const UEdGraphPin* P, TMap<FString, int32>& Seen)
    {
        const FString Base = NodeId + TEXT(":") +
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

    void WriteNode(const FJsonWriterRef& W, UEdGraphNode* N, const FString& NodeId, const TMap<const UEdGraphPin*, FString>& PinIds)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("guid"), NodeId);
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

    void WriteComment(const FJsonWriterRef& W, UEdGraphNode_Comment* C, const FString& NodeId)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("guid"), NodeId);
        W->WriteValue(TEXT("text"), C->NodeComment);
        W->WriteValue(TEXT("x"), C->NodePosX);
        W->WriteValue(TEXT("y"), C->NodePosY);
        W->WriteValue(TEXT("width"), C->NodeWidth);
        W->WriteValue(TEXT("height"), C->NodeHeight);
        W->WriteObjectEnd();
    }

    /** Assign each node its id, sorted by (id, object name), with `#n` suffixes on any duplicate
     *  id so two nodes can never collapse into one in the diff. */
    template <typename NodeT>
    TArray<TPair<NodeT*, FString>> IdentifyNodes(const TArray<NodeT*>& In, const FNodeIdFn& NodeId)
    {
        TArray<TPair<NodeT*, FString>> Out;
        Out.Reserve(In.Num());
        for (NodeT* N : In) Out.Add({ N, NodeId(N) });
        Out.Sort([](const TPair<NodeT*, FString>& A, const TPair<NodeT*, FString>& B)
        {
            return A.Value != B.Value ? A.Value < B.Value : A.Key->GetName() < B.Key->GetName();
        });
        TMap<FString, int32> Seen;
        for (TPair<NodeT*, FString>& E : Out)
        {
            int32& Count = Seen.FindOrAdd(E.Value);
            if (Count > 0) E.Value = FString::Printf(TEXT("%s#%d"), *E.Value, Count);
            ++Count;
        }
        return Out;
    }

    void WriteGraph(const FJsonWriterRef& W, const FGraphEntry& Entry, const FNodeIdFn& NodeId)
    {
        W->WriteObjectStart();
        W->WriteValue(TEXT("name"), Entry.Name);
        W->WriteValue(TEXT("type"), Entry.Type);

        // Split comment boxes from real nodes; keep each list deterministically ordered.
        TArray<UEdGraphNode*> RawNodes;
        TArray<UEdGraphNode_Comment*> RawComments;
        for (UEdGraphNode* N : Entry.Graph->Nodes)
        {
            if (!N) continue;
            if (UEdGraphNode_Comment* C = Cast<UEdGraphNode_Comment>(N)) { RawComments.Add(C); }
            else { RawNodes.Add(N); }
        }
        const TArray<TPair<UEdGraphNode*, FString>> Nodes = IdentifyNodes(RawNodes, NodeId);
        const TArray<TPair<UEdGraphNode_Comment*, FString>> Comments = IdentifyNodes(RawComments, NodeId);

        // Assign every pin in the graph a stable id up front, so both a pin's own id and the
        // link references pointing at it resolve to the same value.
        TMap<const UEdGraphPin*, FString> PinIds;
        TMap<FString, int32> Seen;
        for (const TPair<UEdGraphNode*, FString>& N : Nodes)
        {
            for (const UEdGraphPin* P : N.Key->Pins) { if (P) PinIds.Add(P, StablePinId(N.Value, P, Seen)); }
        }

        W->WriteArrayStart(TEXT("nodes"));
        for (const TPair<UEdGraphNode*, FString>& N : Nodes) WriteNode(W, N.Key, N.Value, PinIds);
        W->WriteArrayEnd();

        W->WriteArrayStart(TEXT("comments"));
        for (const TPair<UEdGraphNode_Comment*, FString>& C : Comments) WriteComment(W, C.Key, C.Value);
        W->WriteArrayEnd();

        W->WriteObjectEnd();
    }

    /** Stable graph order by `type:name`; a repeated key gets a `#n` suffix (the diff matches
     *  graphs by that key, so it must be unique). */
    void SortAndDedupeGraphs(TArray<FGraphEntry>& Graphs)
    {
        Graphs.StableSort([](const FGraphEntry& A, const FGraphEntry& B)
        {
            return (A.Type + TEXT(":") + A.Name) < (B.Type + TEXT(":") + B.Name);
        });
        TMap<FString, int32> Seen;
        for (FGraphEntry& G : Graphs)
        {
            int32& Count = Seen.FindOrAdd(G.Type + TEXT(":") + G.Name);
            if (Count > 0) G.Name = FString::Printf(TEXT("%s#%d"), *G.Name, Count);
            ++Count;
        }
    }

    // ── Blueprints ───────────────────────────────────────────────────────────

    void GatherBlueprintGraphs(UBlueprint* BP, TArray<FGraphEntry>& Out)
    {
        for (UEdGraph* G : BP->UbergraphPages)          if (G) Out.Add({ G, TEXT("Ubergraph"), G->GetName() });
        for (UEdGraph* G : BP->FunctionGraphs)          if (G) Out.Add({ G, TEXT("Function"), G->GetName() });
        for (UEdGraph* G : BP->MacroGraphs)             if (G) Out.Add({ G, TEXT("Macro"), G->GetName() });
        for (UEdGraph* G : BP->DelegateSignatureGraphs) if (G) Out.Add({ G, TEXT("Delegate"), G->GetName() });
    }

    // ── Materials ────────────────────────────────────────────────────────────

    // A material's UMaterialGraph only exists while the material editor has it open (and even
    // then it belongs to the editor's transient copy), so it's rebuilt here exactly the way the
    // editor builds it. RebuildGraph points each expression's transient `GraphNode` (and
    // `SubgraphExpression`) at the new nodes; this scope records and restores them so exporting
    // never leaves the real asset referencing a throwaway graph.
    class FTransientMaterialGraph
    {
    public:
        explicit FTransientMaterialGraph(UObject* Asset)
        {
            UMaterialFunction* Function = Cast<UMaterialFunction>(Asset);
            UMaterial* Host = Cast<UMaterial>(Asset);
            if (Function)
            {
                // Same as FMaterialEditor::InitEditorForMaterialFunction: a function's graph is
                // hosted by a scratch material that borrows the function's expressions.
                Host = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
                Host->AssignExpressionCollection(Function->GetExpressionCollection());
            }
            if (!Host) return;

            for (UMaterialExpression* E : Host->GetExpressions())    { if (E) Saved.Add({ E, E->GraphNode, E->SubgraphExpression }); }
            for (UMaterialExpressionComment* C : Host->GetEditorComments()) { if (C) Saved.Add({ C, C->GraphNode, C->SubgraphExpression }); }

            Graph = NewObject<UMaterialGraph>(GetTransientPackage(), NAME_None, RF_Transient);
            Graph->Schema = UMaterialGraphSchema::StaticClass();
            Graph->Material = Host;
            Graph->MaterialFunction = Function;
            Graph->RebuildGraph();
        }

        ~FTransientMaterialGraph()
        {
            for (const FSavedExpression& S : Saved)
            {
                if (UMaterialExpression* E = S.Expression.Get())
                {
                    E->GraphNode = S.GraphNode;
                    E->SubgraphExpression = S.SubgraphExpression;
                }
            }
            if (Graph) Graph->MarkAsGarbage();
        }

        UMaterialGraph* Graph = nullptr;

    private:
        struct FSavedExpression
        {
            TWeakObjectPtr<UMaterialExpression> Expression;
            TObjectPtr<UEdGraphNode> GraphNode;
            TObjectPtr<UMaterialExpression> SubgraphExpression;
        };
        TArray<FSavedExpression> Saved;
    };

    /** Collapsed-node (composite) subgraphs, named by their path from the top graph. */
    void GatherMaterialSubgraphs(UEdGraph* G, const FString& Prefix, TArray<FGraphEntry>& Out)
    {
        for (UEdGraph* Sub : G->SubGraphs)
        {
            if (!Sub) continue;
            const FString Name = Prefix + TEXT("/") + Sub->GetName();
            Out.Add({ Sub, TEXT("MaterialSubgraph"), Name });
            GatherMaterialSubgraphs(Sub, Name, Out);
        }
    }

    // ── Niagara ──────────────────────────────────────────────────────────────

    bool IsDefaultObjectName(const UObject* O)
    {
        const FString Prefix = O->GetClass()->GetName() + TEXT("_");
        const FString Name = O->GetName();
        return Name.StartsWith(Prefix) && Name.RightChop(Prefix.Len()).IsNumeric();
    }

    /** Objects Niagara keeps only as inheritance-merge baselines — an emitter's
     *  `VersionedParentAtLastMerge` copy and its `ParentScratchPads`. They're internal snapshots
     *  of the parent emitter, not the artist's content; exporting them would duplicate every
     *  inherited graph (and show each change twice). Found by reflection so Niagara isn't linked. */
    TSet<const UObject*> FindNiagaraMergeBaselines(const TArray<UObject*>& Objects)
    {
        static const FName BaselineProps[] = { TEXT("VersionedParentAtLastMerge"), TEXT("ParentScratchPads") };
        const auto IsBaselineProp = [](const FProperty* P)
        {
            return P && (P->GetFName() == BaselineProps[0] || P->GetFName() == BaselineProps[1]);
        };

        TSet<const UObject*> Out;
        for (UObject* O : Objects)
        {
            for (TPropertyValueIterator<FObjectPropertyBase> It(O->GetClass(), O); It; ++It)
            {
                TArray<const FProperty*> Chain;
                It.GetPropertyChain(Chain);
                if (!IsBaselineProp(It.Key()) && !Chain.ContainsByPredicate(IsBaselineProp)) continue;
                if (const UObject* Ref = It.Key()->GetObjectPropertyValue(It->Value)) Out.Add(Ref);
            }
        }
        return Out;
    }

    // Niagara graphs (UNiagaraGraph : UEdGraph) are saved editor-only subobjects: the system
    // script graph under the system's spawn script, one graph per emitter under that emitter,
    // one per script version, plus the system overview graph. Rather than link NiagaraEditor
    // (whose script-source class isn't exported), take every saved UEdGraph in the package and
    // name it by its owner chain — e.g. type `NiagaraEmitter`, name `Sparks`.
    void GatherPackageGraphs(UObject* Asset, TArray<FGraphEntry>& Out)
    {
        TArray<UObject*> Objects;
        GetObjectsWithPackage(Asset->GetOutermost(), Objects, /*bIncludeNestedObjects*/ true,
            RF_Transient, EInternalObjectFlags::Garbage);
        const TSet<const UObject*> Baselines = FindNiagaraMergeBaselines(Objects);
        for (UObject* O : Objects)
        {
            UEdGraph* G = Cast<UEdGraph>(O);
            if (!IsValid(G)) continue;

            TArray<FString> Segments;
            FString Type;
            bool bValidChain = true;
            for (UObject* Outer = G->GetOuter(); Outer && Outer != Asset && !Outer->IsA<UPackage>(); Outer = Outer->GetOuter())
            {
                if (!IsValid(Outer) || Baselines.Contains(Outer)) { bValidChain = false; break; }
                if (Outer->GetClass()->GetName().EndsWith(TEXT("ScriptSource"))) continue; // plumbing, not a name
                if (Type.IsEmpty()) Type = Outer->GetClass()->GetName();
                Segments.Insert(Outer->GetName(), 0);
            }
            if (!bValidChain) continue;
            if (Type.IsEmpty()) Type = Asset->GetClass()->GetName();
            if (Segments.Num() == 0 || !IsDefaultObjectName(G)) Segments.Add(G->GetName());
            Out.Add({ G, Type, FString::Join(Segments, TEXT("/")) });
        }
    }

    // ── Properties ───────────────────────────────────────────────────────────
    // A reflection dump of an asset's editable data, so assets with no node graph (an Enum, an
    // Input Action, a Texture's settings…) still diff as something readable, and graph assets
    // additionally diff their class defaults. Hand-rolled rather than FJsonObjectConverter for
    // two reasons: that converter follows every instanced object reference with no cycle guard
    // (an actor CDO's components point at each other, which would recurse forever and take the
    // editor down from a save hook), and it has no size budget. Output is deterministic:
    // properties come out in reflection order, map keys sorted, floats rounded to what the
    // Details panel shows, so a re-export without edits is byte-identical.

    /** Properties never worth diffing: regenerated on every save or never user-authored. */
    const int64 SkipPropertyFlags = CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient | CPF_Deprecated;

    /** Named properties that change on every save without any authored edit. */
    bool IsVolatileProperty(const FProperty* P)
    {
        static const FName Volatile[] = { TEXT("LightingGuid"), TEXT("StateId"), TEXT("ParameterStateId") };
        for (const FName& N : Volatile) { if (P->GetFName() == N) return true; }
        return false;
    }

    struct FDumpState
    {
        /** Package the dump root lives in; only its own subobjects are expanded inline. */
        const UPackage* Package = nullptr;
        /** Objects on the current expansion path (cycle guard). */
        TArray<const UObject*> Path;
        /** Values emitted so far, against `MaxValues` — a hard ceiling on export size. */
        int32 Emitted = 0;
        bool bTruncated = false;
        static constexpr int32 MaxValues = 60000;
        static constexpr int32 MaxDepth = 10;

        bool Budget() { if (Emitted >= MaxValues) { bTruncated = true; return false; } ++Emitted; return true; }
    };

    TSharedPtr<FJsonValue> PropertyToJson(const FProperty* P, const void* ValuePtr, FDumpState& S);

    /** A float the way the editor prints it (7 significant digits) rather than its double expansion. */
    double RoundedFloat(float F)
    {
        return FCString::Atod(*FString::Printf(TEXT("%.7g"), (double)F));
    }

    FString ExportTextOf(const FProperty* P, const void* ValuePtr)
    {
        FString Out;
        P->ExportText_Direct(Out, ValuePtr, ValuePtr, nullptr, PPF_None);
        return Out;
    }

    /** Should a referenced object be expanded inline (instanced data authored as part of this
     *  asset) or written as a path string (a reference to something else)? */
    bool ShouldExpandObject(const FProperty* P, const UObject* Obj, const FDumpState& S)
    {
        if (!Obj || !S.Package || S.Path.Num() >= FDumpState::MaxDepth) return false;
        if (Obj->GetOutermost() != S.Package) return false;               // another asset: reference only
        if (S.Path.Contains(Obj)) return false;                            // cycle
        if (Obj->IsA<UEdGraph>() || Obj->IsA<UEdGraphNode>()) return false; // graphs are exported separately
        if (Obj->IsA<UStruct>()) return false;                             // classes/structs: a name is enough
        const bool bInstancedProperty = P && P->HasAnyPropertyFlags(CPF_PersistentInstance | CPF_InstancedReference);
        const bool bInstancedClass = Obj->GetClass()->HasAnyClassFlags(CLASS_DefaultToInstanced);
        return bInstancedProperty || bInstancedClass;
    }

    /** The reflected properties of `Def` at `Data`, as a JSON object in reflection order. */
    TSharedRef<FJsonObject> StructToJson(const UStruct* Def, const void* Data, FDumpState& S)
    {
        TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
        for (TFieldIterator<FProperty> It(Def); It; ++It)
        {
            const FProperty* P = *It;
            if (P->HasAnyPropertyFlags(SkipPropertyFlags) || IsVolatileProperty(P)) continue;
            if (!S.Budget()) break;
            const void* ValuePtr = P->ContainerPtrToValuePtr<void>(Data);
            TSharedPtr<FJsonValue> V = PropertyToJson(P, ValuePtr, S);
            if (V.IsValid()) Obj->SetField(P->GetAuthoredName(), V); // authored: a Struct field's friendly name
        }
        return Obj;
    }

    TSharedPtr<FJsonValue> ObjectToJson(const FProperty* P, const UObject* Obj, FDumpState& S)
    {
        if (!Obj) return MakeShared<FJsonValueNull>();
        if (!ShouldExpandObject(P, Obj, S)) return MakeShared<FJsonValueString>(Obj->GetPathName());
        S.Path.Push(Obj);
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
        Out->SetStringField(TEXT("_class"), Obj->GetClass()->GetName());
        Out->SetStringField(TEXT("_name"), Obj->GetName());
        const TSharedRef<FJsonObject> Fields = StructToJson(Obj->GetClass(), Obj, S);
        for (const TPair<FString, TSharedPtr<FJsonValue>>& F : Fields->Values) Out->SetField(F.Key, F.Value);
        S.Path.Pop();
        return MakeShared<FJsonValueObject>(Out);
    }

    TSharedPtr<FJsonValue> PropertyToJson(const FProperty* P, const void* ValuePtr, FDumpState& S)
    {
        if (const FBoolProperty* BP = CastField<FBoolProperty>(P))
        {
            return MakeShared<FJsonValueBoolean>(BP->GetPropertyValue(ValuePtr));
        }
        if (const FEnumProperty* EP = CastField<FEnumProperty>(P))
        {
            const int64 V = EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
            return MakeShared<FJsonValueString>(EP->GetEnum() ? EP->GetEnum()->GetNameStringByValue(V) : FString::Printf(TEXT("%lld"), V));
        }
        if (const FNumericProperty* NP = CastField<FNumericProperty>(P))
        {
            if (const UEnum* E = NP->GetIntPropertyEnum())
            {
                return MakeShared<FJsonValueString>(E->GetNameStringByValue(NP->GetSignedIntPropertyValue(ValuePtr)));
            }
            if (const FFloatProperty* FP = CastField<FFloatProperty>(P))
            {
                return MakeShared<FJsonValueNumber>(RoundedFloat(FP->GetPropertyValue(ValuePtr)));
            }
            if (NP->IsFloatingPoint()) return MakeShared<FJsonValueNumber>(NP->GetFloatingPointPropertyValue(ValuePtr));
            if (NP->IsInteger())
            {
                // JSON numbers are doubles; anything past 2^53 goes out as text to stay exact.
                const int64 V = NP->GetSignedIntPropertyValue(ValuePtr);
                if (V > (1LL << 53) || V < -(1LL << 53)) return MakeShared<FJsonValueString>(FString::Printf(TEXT("%lld"), V));
                return MakeShared<FJsonValueNumber>((double)V);
            }
            return MakeShared<FJsonValueString>(ExportTextOf(P, ValuePtr));
        }
        if (const FStrProperty* SP = CastField<FStrProperty>(P)) return MakeShared<FJsonValueString>(SP->GetPropertyValue(ValuePtr));
        if (const FNameProperty* NmP = CastField<FNameProperty>(P)) return MakeShared<FJsonValueString>(NmP->GetPropertyValue(ValuePtr).ToString());
        if (const FTextProperty* TP = CastField<FTextProperty>(P)) return MakeShared<FJsonValueString>(TP->GetPropertyValue(ValuePtr).ToString());
        if (const FStructProperty* StP = CastField<FStructProperty>(P))
        {
            // Structs with a native text form (FGuid, FSoftObjectPath, FDateTime…) export as that one
            // string; everything else (FVector, FLinearColor, user structs) expands field by field.
            const UScriptStruct* SS = StP->Struct;
            if (SS && SS->GetCppStructOps() && SS->GetCppStructOps()->HasExportTextItem())
            {
                return MakeShared<FJsonValueString>(ExportTextOf(P, ValuePtr));
            }
            return MakeShared<FJsonValueObject>(StructToJson(SS, ValuePtr, S));
        }
        if (const FArrayProperty* AP = CastField<FArrayProperty>(P))
        {
            FScriptArrayHelper Helper(AP, ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Out;
            Out.Reserve(Helper.Num());
            for (int32 i = 0; i < Helper.Num(); ++i)
            {
                if (!S.Budget()) break;
                TSharedPtr<FJsonValue> V = PropertyToJson(AP->Inner, Helper.GetRawPtr(i), S);
                Out.Add(V.IsValid() ? V : MakeShared<FJsonValueNull>());
            }
            return MakeShared<FJsonValueArray>(Out);
        }
        if (const FSetProperty* SetP = CastField<FSetProperty>(P))
        {
            FScriptSetHelper Helper(SetP, ValuePtr);
            TArray<TSharedPtr<FJsonValue>> Out;
            for (FScriptSetHelper::FIterator It(Helper); It; ++It)
            {
                if (!S.Budget()) break;
                TSharedPtr<FJsonValue> V = PropertyToJson(SetP->ElementProp, Helper.GetElementPtr(It), S);
                Out.Add(V.IsValid() ? V : MakeShared<FJsonValueNull>());
            }
            // Set order is hash order, not authored order — sort by text so re-exports are stable.
            Out.Sort([](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B)
            {
                FString SA, SB; A->TryGetString(SA); B->TryGetString(SB); return SA < SB;
            });
            return MakeShared<FJsonValueArray>(Out);
        }
        if (const FMapProperty* MP = CastField<FMapProperty>(P))
        {
            FScriptMapHelper Helper(MP, ValuePtr);
            TArray<TPair<FString, TSharedPtr<FJsonValue>>> Pairs;
            for (FScriptMapHelper::FIterator It(Helper); It; ++It)
            {
                if (!S.Budget()) break;
                const FString Key = ExportTextOf(MP->KeyProp, Helper.GetKeyPtr(It));
                TSharedPtr<FJsonValue> V = PropertyToJson(MP->ValueProp, Helper.GetValuePtr(It), S);
                Pairs.Add({ Key, V.IsValid() ? V : MakeShared<FJsonValueNull>() });
            }
            Pairs.Sort([](const TPair<FString, TSharedPtr<FJsonValue>>& A, const TPair<FString, TSharedPtr<FJsonValue>>& B) { return A.Key < B.Key; });
            TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Pairs) Out->SetField(Pair.Key, Pair.Value);
            return MakeShared<FJsonValueObject>(Out);
        }
        if (const FSoftObjectProperty* SoP = CastField<FSoftObjectProperty>(P))
        {
            const FSoftObjectPtr& Soft = SoP->GetPropertyValue(ValuePtr);
            return Soft.IsNull() ? TSharedPtr<FJsonValue>(MakeShared<FJsonValueNull>()) : MakeShared<FJsonValueString>(Soft.ToString());
        }
        if (const FObjectPropertyBase* OP = CastField<FObjectPropertyBase>(P))
        {
            return ObjectToJson(P, OP->GetObjectPropertyValue(ValuePtr), S);
        }
        if (const FInterfaceProperty* IP = CastField<FInterfaceProperty>(P))
        {
            const UObject* Obj = IP->GetPropertyValue(ValuePtr).GetObject();
            return Obj ? TSharedPtr<FJsonValue>(MakeShared<FJsonValueString>(Obj->GetPathName())) : MakeShared<FJsonValueNull>();
        }
        if (CastField<FDelegateProperty>(P) || CastField<FMulticastDelegateProperty>(P))
        {
            return nullptr; // bindings are code wiring, not authored data
        }
        return MakeShared<FJsonValueString>(ExportTextOf(P, ValuePtr));
    }

    /** `Obj`'s own properties as a JSON object (the common case: "dump this asset"). */
    TSharedRef<FJsonObject> DumpObject(const UObject* Obj, FDumpState& S)
    {
        if (!Obj) return MakeShared<FJsonObject>();
        S.Path.Push(Obj);
        TSharedRef<FJsonObject> Out = StructToJson(Obj->GetClass(), Obj, S);
        S.Path.Pop();
        return Out;
    }

    // Per-kind property sections. Each returns the `properties` object for one asset kind; the
    // generic fallback is the asset's own property values, flat.

    TSharedRef<FJsonObject> EnumProperties(const UEnum* E)
    {
        TSharedRef<FJsonObject> Enumerators = MakeShared<FJsonObject>();
        const int32 Count = E->NumEnums() - (E->ContainsExistingMax() ? 1 : 0);
        for (int32 i = 0; i < Count; ++i)
        {
            TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
            Entry->SetStringField(TEXT("displayName"), E->GetDisplayNameTextByIndex(i).ToString());
            Entry->SetNumberField(TEXT("value"), (double)E->GetValueByIndex(i));
            // Keyed by the internal name, which survives a display-name edit — so renaming an
            // entry diffs as "displayName changed", not as one entry removed and another added.
            Enumerators->SetObjectField(E->GetNameStringByIndex(i), Entry);
        }
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
        Out->SetObjectField(TEXT("enumerators"), Enumerators);
        return Out;
    }

    TSharedRef<FJsonObject> StructProperties(const UUserDefinedStruct* St, FDumpState& S)
    {
        TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
        for (TFieldIterator<FProperty> It(St); It; ++It)
        {
            TSharedRef<FJsonObject> F = MakeShared<FJsonObject>();
            F->SetStringField(TEXT("type"), It->GetCPPType());
            Fields->SetObjectField(It->GetAuthoredName(), F);
        }
        // InitializeStruct applies the authored default values, so a scratch instance is the defaults.
        FStructOnScope Scratch(St);
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
        Out->SetObjectField(TEXT("fields"), Fields);
        Out->SetObjectField(TEXT("defaults"), StructToJson(St, Scratch.GetStructMemory(), S));
        return Out;
    }

    TSharedRef<FJsonObject> DataTableProperties(const UDataTable* Table, FDumpState& S)
    {
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
        const UScriptStruct* RowStruct = Table->GetRowStruct();
        if (RowStruct) Out->SetStringField(TEXT("rowStruct"), RowStruct->GetPathName());
        else Out->SetField(TEXT("rowStruct"), MakeShared<FJsonValueNull>());

        TArray<FName> Names;
        Table->GetRowMap().GetKeys(Names);
        Names.Sort(FNameLexicalLess());
        TSharedRef<FJsonObject> Rows = MakeShared<FJsonObject>();
        for (const FName& Name : Names)
        {
            if (!S.Budget()) break;
            uint8* const* Row = Table->GetRowMap().Find(Name);
            if (RowStruct && Row && *Row) Rows->SetObjectField(Name.ToString(), StructToJson(RowStruct, *Row, S));
        }
        Out->SetObjectField(TEXT("rows"), Rows);
        return Out;
    }

    TSharedRef<FJsonObject> BlueprintProperties(UBlueprint* BP, FDumpState& S)
    {
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

        // Declared variables (name → type/category/default). Keyed by name so a retype or a
        // default edit diffs on the variable, not on a list index.
        TSharedRef<FJsonObject> Variables = MakeShared<FJsonObject>();
        for (const FBPVariableDescription& V : BP->NewVariables)
        {
            TSharedRef<FJsonObject> Var = MakeShared<FJsonObject>();
            Var->SetStringField(TEXT("type"), PinTypeString(V.VarType));
            Var->SetStringField(TEXT("category"), V.Category.ToString());
            Var->SetStringField(TEXT("defaultValue"), V.DefaultValue);
            Var->SetBoolField(TEXT("instanceEditable"), !(V.PropertyFlags & CPF_DisableEditOnInstance));
            Variables->SetObjectField(V.VarName.ToString(), Var);
        }
        Out->SetObjectField(TEXT("variables"), Variables);

        // Components added in the Blueprint editor (the SCS tree), each with its template's values.
        TSharedRef<FJsonObject> Components = MakeShared<FJsonObject>();
        if (BP->SimpleConstructionScript)
        {
            TMap<const USCS_Node*, FName> ParentOf;
            for (const USCS_Node* N : BP->SimpleConstructionScript->GetAllNodes())
            {
                if (!N) continue;
                for (const USCS_Node* C : N->GetChildNodes()) { if (C) ParentOf.Add(C, N->GetVariableName()); }
            }
            for (const USCS_Node* N : BP->SimpleConstructionScript->GetAllNodes())
            {
                if (!N) continue;
                TSharedRef<FJsonObject> Comp = MakeShared<FJsonObject>();
                Comp->SetStringField(TEXT("class"), N->ComponentClass ? N->ComponentClass->GetName() : FString());
                if (const FName* Parent = ParentOf.Find(N)) Comp->SetStringField(TEXT("parent"), Parent->ToString());
                else if (!N->ParentComponentOrVariableName.IsNone()) Comp->SetStringField(TEXT("parent"), N->ParentComponentOrVariableName.ToString());
                else Comp->SetField(TEXT("parent"), MakeShared<FJsonValueNull>());
                if (!N->AttachToName.IsNone()) Comp->SetStringField(TEXT("socket"), N->AttachToName.ToString());
                Comp->SetObjectField(TEXT("properties"), DumpObject(N->ComponentTemplate, S));
                Components->SetObjectField(N->GetVariableName().ToString(), Comp);
            }
        }
        Out->SetObjectField(TEXT("components"), Components);

        // Class defaults: every value on the Details panel of the Blueprint itself.
        const UObject* CDO = BP->GeneratedClass ? BP->GeneratedClass->GetDefaultObject(/*bCreateIfNeeded*/ false) : nullptr;
        Out->SetObjectField(TEXT("defaults"), DumpObject(CDO, S));
        return Out;
    }

    TSharedRef<FJsonObject> WorldProperties(UWorld* World, FDumpState& S)
    {
        TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
        AWorldSettings* Settings = World->GetWorldSettings(/*bCheckStreamingPersistent*/ false, /*bChecked*/ false);
        Out->SetObjectField(TEXT("worldSettings"), DumpObject(Settings, S));
        return Out;
    }

    /** The `properties` object for any asset. */
    TSharedRef<FJsonObject> BuildProperties(UObject* Asset, FDumpState& S)
    {
        S.Package = Asset->GetOutermost();
        if (const UUserDefinedEnum* E = Cast<UUserDefinedEnum>(Asset)) return EnumProperties(E);
        if (const UUserDefinedStruct* St = Cast<UUserDefinedStruct>(Asset)) return StructProperties(St, S);
        if (const UDataTable* Table = Cast<UDataTable>(Asset)) return DataTableProperties(Table, S);
        if (UBlueprint* BP = Cast<UBlueprint>(Asset)) return BlueprintProperties(BP, S);
        if (UWorld* World = Cast<UWorld>(Asset)) return WorldProperties(World, S);
        return DumpObject(Asset, S);
    }

    /** The Blueprint whose graphs a level package carries: its Level Blueprint, if one was ever made. */
    UBlueprint* LevelBlueprintOf(UWorld* World)
    {
        return World && World->PersistentLevel ? World->PersistentLevel->GetLevelScriptBlueprint(/*bDontCreate*/ true) : nullptr;
    }
}

bool FKeystoneBlueprintExporter::IsExcludedPackage(const FString& PackageName)
{
    // One File Per Actor: every placed actor is its own tiny package. Exporting them would mean
    // thousands of files per level, and loading one standalone pulls in its whole level.
    return PackageName.Contains(TEXT("/__ExternalActors__/")) || PackageName.Contains(TEXT("/__ExternalObjects__/"));
}

bool FKeystoneBlueprintExporter::CanExport(const UObject* Asset)
{
    if (!IsValid(Asset) || Asset->IsA<UPackage>() || Asset->IsA<UObjectRedirector>()) return false;
    if (Asset->HasAnyFlags(RF_ClassDefaultObject | RF_Transient)) return false;
    const UPackage* Package = Asset->GetOutermost();
    if (!Package || Package == GetTransientPackage()) return false;
    return !IsExcludedPackage(Package->GetName());
}

FString FKeystoneBlueprintExporter::BuildJson(UObject* Asset)
{
    FString Out;
    const FJsonWriterRef W = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);

    W->WriteObjectStart();
    W->WriteValue(TEXT("version"), KEYSTONE_EXPORT_VERSION);
    W->WriteValue(TEXT("generatedBy"), TEXT("keystone-blueprint-export"));
    W->WriteValue(TEXT("package"), Asset->GetOutermost()->GetName()); // /Game/.../BP_Hero
    W->WriteValue(TEXT("asset"), Asset->GetName());
    W->WriteValue(TEXT("assetClass"), Asset->GetClass()->GetName());

    TArray<FGraphEntry> Graphs;
    FNodeIdFn NodeId = &DefaultNodeId;
    TUniquePtr<FTransientMaterialGraph> MaterialGraph; // must outlive WriteGraph below

    // A level's graphs are its Level Blueprint's; everything else that has graphs owns them.
    UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
    if (UWorld* World = Cast<UWorld>(Asset)) Blueprint = LevelBlueprintOf(World);

    if (Blueprint)
    {
        if (Blueprint->ParentClass) { W->WriteValue(TEXT("parentClass"), Blueprint->ParentClass->GetName()); }
        else { W->WriteNull(TEXT("parentClass")); }
        W->WriteValue(TEXT("blueprintType"), StaticEnum<EBlueprintType>()->GetNameStringByValue(Blueprint->BlueprintType));
        GatherBlueprintGraphs(Blueprint, Graphs);
    }
    else
    {
        W->WriteNull(TEXT("parentClass"));
        W->WriteNull(TEXT("blueprintType"));

        if (Asset->IsA<UMaterial>() || Asset->IsA<UMaterialFunction>())
        {
            MaterialGraph = MakeUnique<FTransientMaterialGraph>(Asset);
            if (MaterialGraph->Graph)
            {
                const FString Type = Asset->IsA<UMaterial>() ? TEXT("Material") : TEXT("MaterialFunction");
                Graphs.Add({ MaterialGraph->Graph, Type, TEXT("MaterialGraph") });
                GatherMaterialSubgraphs(MaterialGraph->Graph, TEXT("MaterialGraph"), Graphs);
            }
            NodeId = &MaterialNodeId;
        }
        else if (IsNiagaraAsset(Asset))
        {
            GatherPackageGraphs(Asset, Graphs);
        }
    }

    SortAndDedupeGraphs(Graphs);
    W->WriteArrayStart(TEXT("graphs"));
    for (const FGraphEntry& G : Graphs) WriteGraph(W, G, NodeId);
    W->WriteArrayEnd();

    // Properties: the asset's Details-panel data (see BuildProperties). Written after the graphs
    // so the graph half of the file is unchanged from format version 1.
    FDumpState State;
    const TSharedRef<FJsonObject> Properties = BuildProperties(Asset, State);
    if (State.bTruncated) Properties->SetBoolField(TEXT("_truncated"), true);
    FJsonSerializer::Serialize(MakeShared<FJsonValueObject>(Properties), TEXT("properties"), W, /*bCloseWriter*/ false);

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

FString FKeystoneBlueprintExporter::ExportFileFor(UObject* Asset)
{
    return ExportFileForPackage(Asset->GetOutermost()->GetName());
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

bool FKeystoneBlueprintExporter::ExportOne(UObject* Asset, FKeystoneExportResult& InOutResult)
{
    if (!CanExport(Asset)) { InOutResult.Failed++; return false; }
    InOutResult.Scanned++;

    const FString Json = BuildJson(Asset);
    const FString File = ExportFileFor(Asset);

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
    // miss assets, and the prune would have to keep every file it cannot account for. This is
    // a manual, one-off action, so blocking until the scan finishes is acceptable.
    if (ARM.Get().IsLoadingAssets())
    {
        ARM.Get().WaitForCompletion();
    }

    // Every asset under the root. No class filter: anything with a Details panel gets an export.
    FARFilter Filter;
    Filter.PackagePaths.Add(*RootPath);
    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;
    ARM.Get().GetAssets(Filter, Assets);

    // One export per package: drop redirectors, the One-File-Per-Actor folders, and any second
    // asset sharing a package with the first (the export is keyed by package).
    Assets.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.PackageName != B.PackageName ? A.PackageName.LexicalLess(B.PackageName) : A.AssetName.LexicalLess(B.AssetName);
    });
    TArray<FAssetData> Unique;
    Unique.Reserve(Assets.Num());
    FName LastPackage;
    for (const FAssetData& AD : Assets)
    {
        if (AD.IsRedirector() || AD.PackageName == LastPackage) continue;
        if (IsExcludedPackage(AD.PackageName.ToString())) continue;
        Unique.Add(AD);
        LastPackage = AD.PackageName;
    }

    // Manifest: package -> committed export path, so Keystone can resolve a changed .uasset to
    // its .bpgraph.json without re-deriving the path rule, and can tell which plugin generation
    // wrote the tree (version 2 = every asset has an export, with properties).
    FString ManifestJson;
    const FJsonWriterRef MW = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&ManifestJson);
    MW->WriteObjectStart();
    MW->WriteValue(TEXT("version"), KEYSTONE_EXPORT_VERSION);
    MW->WriteValue(TEXT("generatedBy"), TEXT("keystone-blueprint-export"));
    MW->WriteValue(TEXT("project"), FApp::GetProjectName());
    MW->WriteArrayStart(TEXT("entries"));

    // Every export a live asset maps to. Anything else under the folder is an orphan left
    // behind by a rename or a delete, which the sweep prunes below.
    TSet<FString> ExpectedFiles;
    ExpectedFiles.Reserve(Unique.Num());

    int32 LoadedSinceGc = 0;
    for (const FAssetData& AD : Unique)
    {
        UObject* Asset = AD.GetAsset();
        if (!CanExport(Asset))
        {
            UE_LOG(LogTemp, Warning, TEXT("[keystone] could not load %s for export"), *AD.GetObjectPathString());
            R.Failed++;
            continue;
        }
        ExportOne(Asset, R);

        const FString File = ExportFileFor(Asset);
        ExpectedFiles.Add(File);

        FString RepoRel = File;
        FPaths::MakePathRelativeTo(RepoRel, *(FPaths::ProjectDir())); // e.g. BlueprintGraphs/Characters/BP_Hero.bpgraph.json
        MW->WriteObjectStart();
        MW->WriteValue(TEXT("package"), Asset->GetOutermost()->GetName());
        MW->WriteValue(TEXT("asset"), Asset->GetName());
        MW->WriteValue(TEXT("assetClass"), Asset->GetClass()->GetName());
        MW->WriteValue(TEXT("exportPath"), RepoRel);
        MW->WriteObjectEnd();

        // A sweep now loads every asset in the project, meshes and textures included. Release
        // what the loop pulled in every so often so a big project does not exhaust memory;
        // anything open in an editor or otherwise referenced survives the collection.
        if (++LoadedSinceGc >= 64)
        {
            LoadedSinceGc = 0;
            CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
        }
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
