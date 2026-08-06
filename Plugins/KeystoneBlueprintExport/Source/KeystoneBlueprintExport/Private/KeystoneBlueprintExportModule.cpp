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
        SetAutoCapture(true); // "artists do nothing": capture on save is on out of the box
    }

    virtual void ShutdownModule() override
    {
        SetAutoCapture(false);
        UToolMenus::UnRegisterStartupCallback(this);
        if (UObjectInitialized()) UToolMenus::UnregisterOwner(MenuOwner());
    }

private:
    static inline FDelegateHandle SaveHandle;

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

    // ── menu ─────────────────────────────────────────────────────────────────
    static void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(MenuOwner());
        UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
        if (!MainMenu) return;
        FToolMenuSection& Section = MainMenu->FindOrAddSection("Keystone");
        Section.AddSubMenu(
            "Keystone",
            LOCTEXT("KeystoneMenu", "Keystone"),
            LOCTEXT("KeystoneMenuTip", "Export Blueprint graphs for Keystone visual diffs"),
            FNewToolMenuChoice(FNewToolMenuDelegate::CreateStatic(&FKeystoneBlueprintExportModule::BuildMenu)));
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
        S.AddMenuEntry("ToggleAutoCapture",
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
