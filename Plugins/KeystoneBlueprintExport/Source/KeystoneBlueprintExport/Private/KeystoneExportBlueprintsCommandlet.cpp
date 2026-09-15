#include "KeystoneExportBlueprintsCommandlet.h"

#include "KeystoneBlueprintExporter.h"
#include "KeystoneGit.h"

#include "AssetRegistry/AssetRegistryModule.h"

int32 UKeystoneExportBlueprintsCommandlet::Main(const FString& Params)
{
    TArray<FString> Tokens, Switches;
    TMap<FString, FString> ParamVals;
    ParseCommandLine(*Params, Tokens, Switches, ParamVals);

    const FString Root = ParamVals.Contains(TEXT("path")) ? ParamVals[TEXT("path")] : TEXT("/Game");
    UE_LOG(LogTemp, Display, TEXT("[keystone] commandlet exporting Blueprints under %s"), *Root);

    // A commandlet cannot rely on the editor's background asset scan having run. Without a full
    // synchronous search the registry is incomplete: the sweep would export only part of the
    // project, and the prune would have to keep every file it could not account for.
    FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().SearchAllAssets(/*bSynchronousSearch=*/true);

    const FKeystoneExportResult R = FKeystoneBlueprintExporter::ExportAll(Root);
    UE_LOG(LogTemp, Display, TEXT("[keystone] scanned=%d written=%d unchanged=%d failed=%d pruned=%d"),
        R.Scanned, R.Written, R.Unchanged, R.Failed, R.Pruned);

    if (Switches.Contains(TEXT("commit")))
    {
        FString Err;
        if (FKeystoneGit::CommitExports(TEXT("Update Keystone Blueprint graphs (automated)"), Err))
        {
            UE_LOG(LogTemp, Display, TEXT("[keystone] committed + pushed Blueprint graphs"));
        }
        else
        {
            // "nothing to commit" is normal (nothing changed) — warn, don't fail the run.
            UE_LOG(LogTemp, Warning, TEXT("[keystone] commit skipped: %s"), *Err);
        }
    }

    // Non-zero only on genuine export failures, so CI can gate on it.
    return R.Failed > 0 ? 1 : 0;
}
