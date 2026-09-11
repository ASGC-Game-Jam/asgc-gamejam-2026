// Keystone Blueprint Export — editor module. Wires the exporter core to two of its three
// triggers (the third, the commandlet, needs no module glue):
//   • a Keystone menu with plain-language buttons (mirrors the source-art tool's menu), and
//   • auto-capture on save (ON by default) so saving a Blueprint refreshes its .bpgraph.json
//     with zero clicks — artists just work; the file stays current alongside their .uasset.
//
// On-save only *writes* the export next to the asset; pushing is left to the artist's normal
// commit, the "Commit & Push" button, or the commandlet — so a save never forces a network push.
#include "Modules/ModuleManager.h"
#include "KeystoneBlueprintExporter.h"
#include "KeystoneGit.h"

#include "Engine/Blueprint.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/ObjectSaveContext.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Ticker.h"
#include "Misc/ScopeLock.h"
#include "ToolMenus.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "Keystone"

class FKeystoneBlueprintExportModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateStatic(&FKeystoneBlueprintExportModule::RegisterMenus));
        SetAutoCapture(true);      // "artists do nothing": capture on save is on out of the box
        SetLifecycleTracking(true); // …and the matching half: deleting or renaming clears the export
    }

    virtual void ShutdownModule() override
    {
        SetLifecycleTracking(false);
        SetAutoCapture(false);
        UToolMenus::UnRegisterStartupCallback(this);
        if (UObjectInitialized()) UToolMenus::UnregisterOwner(MenuOwner());
    }

private:
    static inline FDelegateHandle SaveHandle;
    static inline FDelegateHandle AssetRemovedHandle;
    static inline FDelegateHandle AssetRenamedHandle;

    /** Stable owner for the menu we add, so register and unregister match (a name owner, not a
     *  fabricated pointer — the latter doesn't convert to FToolMenuOwner in UE5.7). */
    static FToolMenuOwner MenuOwner() { return FName("KeystoneBlueprintExport"); }

    static bool IsAutoCaptureOn() { return SaveHandle.IsValid(); }

    static void SetAutoCapture(bool bOn)
    {
        if (bOn && !SaveHandle.IsValid())
        {
            // UPackage::PackageSavedWithContextEvent — the post-save hook (UE5.0+). If your engine
            // predates it, bind FEditorDelegates::OnPackageSaved instead.
            SaveHandle = UPackage::PackageSavedWithContextEvent.AddStatic(&FKeystoneBlueprintExportModule::OnPackageSaved);
        }
        else if (!bOn && SaveHandle.IsValid())
        {
            UPackage::PackageSavedWithContextEvent.Remove(SaveHandle);
            SaveHandle.Reset();
        }
    }

    static void OnPackageSaved(const FString& /*PackageFilename*/, UPackage* Package, FObjectPostSaveContext /*Context*/)
    {
        if (!Package) return;
        UBlueprint* BP = nullptr;
        ForEachObjectWithPackage(Package, [&BP](UObject* Obj)
        {
            if (UBlueprint* B = Cast<UBlueprint>(Obj)) { BP = B; return false; }
            return true;
        });
        if (!BP) return;
        FKeystoneExportResult R;
        FKeystoneBlueprintExporter::ExportOne(BP, R);
    }

    // ── asset lifecycle ──────────────────────────────────────────────────────
    // The save hook only ever *writes*, so without these a deleted Blueprint left its export
    // behind forever, and a rename left the old path orphaned beside the new one.

    static void SetLifecycleTracking(bool bOn)
    {
        if (bOn && !AssetRemovedHandle.IsValid())
        {
            IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
            AssetRemovedHandle = AR.OnAssetRemoved().AddStatic(&FKeystoneBlueprintExportModule::OnAssetRemoved);
            AssetRenamedHandle = AR.OnAssetRenamed().AddStatic(&FKeystoneBlueprintExportModule::OnAssetRenamed);
        }
        else if (!bOn && AssetRemovedHandle.IsValid())
        {
            // Do not force-load the module during shutdown just to unbind from it.
            if (FAssetRegistryModule* ARM = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
            {
                ARM->Get().OnAssetRemoved().Remove(AssetRemovedHandle);
                ARM->Get().OnAssetRenamed().Remove(AssetRenamedHandle);
            }
            AssetRemovedHandle.Reset();
            AssetRenamedHandle.Reset();

            // A queued check must never fire into an unloaded module.
            FScopeLock Lock(&PendingLock);
            FTSTicker::RemoveTicker(PruneTicker);
            PruneTicker.Reset();
            PendingPrunes.Reset();
        }
    }

    static void OnAssetRemoved(const FAssetData& AssetData)
    {
        // No Blueprint-class filter: checking it would mean loading an asset that is on its way
        // out. If the package never had an export there is simply nothing to delete.
        QueuePrune(AssetData.PackageName.ToString());
    }

    static void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
    {
        // OldObjectPath is an object path (/Game/Path/BP_Name.BP_Name); the export rule is keyed
        // on the package, so drop everything from the first dot.
        FString OldPackage = OldObjectPath;
        int32 DotIndex = INDEX_NONE;
        if (OldPackage.FindChar(TEXT('.'), DotIndex))
        {
            OldPackage.LeftInline(DotIndex);
        }

        QueuePrune(OldPackage);
    }

    // Both events are raised *before* the operation they describe has landed: ObjectTools
    // broadcasts OnAssetRemoved and only afterwards deletes the .uasset, so deciding inside the
    // callback always sees the file and never prunes. They are also thread-safe delegates that
    // may arrive off the game thread. So the callbacks only queue the package, and the decision
    // is made on a later game-thread tick, once the delete or rename has actually finished.

    static constexpr float PruneRetryDelaySeconds = 1.0f;
    static constexpr int32 PruneMaxAttempts = 10;

    static inline FCriticalSection PendingLock;
    static inline TMap<FString, int32> PendingPrunes; // package -> attempts made so far
    static inline FTSTicker::FDelegateHandle PruneTicker;

    static void QueuePrune(const FString& PackageName)
    {
        if (PackageName.IsEmpty()) return;

        FScopeLock Lock(&PendingLock);
        PendingPrunes.FindOrAdd(PackageName, 0);
        if (!PruneTicker.IsValid())
        {
            PruneTicker = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateStatic(&FKeystoneBlueprintExportModule::ProcessPendingPrunes));
        }
    }

    static bool ProcessPendingPrunes(float /*DeltaTime*/)
    {
        TMap<FString, int32> Batch;
        {
            FScopeLock Lock(&PendingLock);
            Batch = MoveTemp(PendingPrunes);
            PendingPrunes.Reset();
            PruneTicker.Reset();
        }

        TMap<FString, int32> Retry;
        for (const TPair<FString, int32>& Entry : Batch)
        {
            switch (FKeystoneBlueprintExporter::ClassifyPackage(Entry.Key))
            {
            case FKeystoneBlueprintExporter::EPackageState::Stale:
                FKeystoneBlueprintExporter::DeleteExportForPackage(Entry.Key);
                break;

            case FKeystoneBlueprintExporter::EPackageState::Unknown:
                // The file is still on disk but the registry cannot account for it — most likely
                // a delete still in flight behind a modal prompt. Try again shortly, then give up
                // and leave it to the Export Blueprint Graphs sweep.
                if (Entry.Value + 1 < PruneMaxAttempts)
                {
                    Retry.Add(Entry.Key, Entry.Value + 1);
                }
                break;

            case FKeystoneBlueprintExporter::EPackageState::Live:
                break;
            }
        }

        if (Retry.Num() > 0)
        {
            FScopeLock Lock(&PendingLock);
            for (const TPair<FString, int32>& Entry : Retry)
            {
                int32& Attempts = PendingPrunes.FindOrAdd(Entry.Key, 0);
                Attempts = FMath::Max(Attempts, Entry.Value);
            }
            if (!PruneTicker.IsValid())
            {
                PruneTicker = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateStatic(&FKeystoneBlueprintExportModule::ProcessPendingPrunes),
                    PruneRetryDelaySeconds);
            }
        }

        return false; // one-shot; re-armed above only while something is still pending
    }

    // ── menu ─────────────────────────────────────────────────────────────────
    static void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(MenuOwner());
        UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
        if (!MainMenu) return;

        // The Keystone menu is shared with the Python source-art tool (Content/Python/keystone_menu.py),
        // which adds a sub-menu entry named "Keystone" to the unnamed section of the menu bar, the
        // same section that holds File and Edit. Register the identical entry in that identical
        // section: a same-named entry in the same section is replaced rather than duplicated, so
        // there is only ever one Keystone menu, and each tool adds its own section to the one
        // sub-menu behind it ("LevelEditor.MainMenu.Keystone").
        //
        // Do not move this entry into its own section or give it a construct delegate. The menu
        // system resolves a sub-menu by entry name and takes the *first* match, which is always the
        // entry in the unnamed section, so a delegate on a second same-named entry is silently never
        // called. That is exactly how Export Blueprint Graphs went missing from this menu.
        UToolMenu* KeystoneMenu = MainMenu->AddSubMenu(
            MenuOwner(),
            NAME_None,
            "Keystone",
            LOCTEXT("KeystoneMenu", "Keystone"),
            LOCTEXT("KeystoneMenuTip", "Keystone tools: Blueprint graph export and source-art sync"));
        if (KeystoneMenu)
        {
            BuildMenu(KeystoneMenu);
        }
    }

    static void BuildMenu(UToolMenu* Menu)
    {
        FToolMenuSection& S = Menu->AddSection("KeystoneActions", LOCTEXT("KeystoneActions", "Blueprint Graphs"));
        S.AddMenuEntry("ExportGraphs",
            LOCTEXT("ExportGraphs", "Export Blueprint Graphs…"),
            LOCTEXT("ExportGraphsTip", "Write every Blueprint's node graph to BlueprintGraphs/*.bpgraph.json"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateStatic(&FKeystoneBlueprintExportModule::OnExportClicked)));
        S.AddMenuEntry("CommitGraphs",
            LOCTEXT("CommitGraphs", "Commit & Push Blueprint Graphs…"),
            LOCTEXT("CommitGraphsTip", "Stage, commit and push only the BlueprintGraphs/ folder"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateStatic(&FKeystoneBlueprintExportModule::OnCommitClicked)));
        S.AddMenuEntry("ToggleGraphAutoCapture",
            LOCTEXT("ToggleAutoCapture", "Toggle Auto-Capture on Save"),
            LOCTEXT("ToggleAutoCaptureTip", "Re-export a Blueprint's graph automatically whenever it's saved"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateStatic(&FKeystoneBlueprintExportModule::OnToggleAutoCapture),
                FCanExecuteAction(),
                FIsActionChecked::CreateStatic(&FKeystoneBlueprintExportModule::IsAutoCaptureOn)),
            EUserInterfaceActionType::ToggleButton);
    }

    static void Info(const FString& Msg) { FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg)); }
    static bool Confirm(const FString& Msg) { return FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(Msg)) == EAppReturnType::Yes; }

    static void OnExportClicked()
    {
        const FKeystoneExportResult R = FKeystoneBlueprintExporter::ExportAll();
        FString Msg = FString::Printf(
            TEXT("Exported Blueprint graphs.\n\nScanned: %d\nWritten/updated: %d\nUnchanged: %d"),
            R.Scanned, R.Written, R.Unchanged);
        if (R.Pruned > 0) Msg += FString::Printf(TEXT("\nRemoved (stale): %d"), R.Pruned);
        if (R.Failed > 0) Msg += FString::Printf(TEXT("\nFailed: %d (see the Output Log)"), R.Failed);
        Msg += TEXT("\n\nNext: Keystone ▸ Commit & Push Blueprint Graphs.");
        Info(Msg);
    }

    static void OnCommitClicked()
    {
        if (!FKeystoneGit::Available())
        {
            Info(TEXT("Git isn't on this machine's PATH.\nCommit BlueprintGraphs/ with your usual git tool."));
            return;
        }
        if (!Confirm(TEXT("Commit and push the BlueprintGraphs/ folder to the remote?\n\n(Only that path is staged — your other changes are untouched.)")))
            return;
        FString Err;
        if (FKeystoneGit::CommitExports(TEXT("Update Keystone Blueprint graphs (automated)"), Err))
            Info(TEXT("Blueprint graphs committed and pushed. Keystone will pick them up on its next sync."));
        else
            Info(Err);
    }

    static void OnToggleAutoCapture()
    {
        const bool bNext = !IsAutoCaptureOn();
        SetAutoCapture(bNext);
        Info(bNext
            ? TEXT("Auto-capture ON — saving a Blueprint re-exports its graph automatically.")
            : TEXT("Auto-capture OFF — export manually with Keystone ▸ Export Blueprint Graphs."));
    }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKeystoneBlueprintExportModule, KeystoneBlueprintExport)
