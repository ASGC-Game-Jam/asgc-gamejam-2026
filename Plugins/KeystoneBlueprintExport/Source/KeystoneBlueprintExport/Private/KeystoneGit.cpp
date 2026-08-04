#include "KeystoneGit.h"

#include "KeystoneBlueprintExporter.h"
#include "Misc/Paths.h"
#include "Misc/MonitoredProcess.h"
#include "HAL/PlatformProcess.h"

int32 FKeystoneGit::Run(const FString& Args, const FString& WorkingDir, FString& Out)
{
    int32 ReturnCode = -1;
    FString StdOut, StdErr;
    // Blocking process capture — export/commit runs are short and user-initiated.
    const bool bLaunched = FPlatformProcess::ExecProcess(
        TEXT("git"), *Args, &ReturnCode, &StdOut, &StdErr, *WorkingDir);
    Out = StdOut + StdErr;
    if (!bLaunched)
    {
        Out = TEXT("could not launch git (is it on PATH?)");
        return -1;
    }
    return ReturnCode;
}

bool FKeystoneGit::Available()
{
    FString Out;
    return Run(TEXT("--version"), FPaths::ProjectDir(), Out) == 0;
}

FString FKeystoneGit::RepoRoot()
{
    FString Out;
    if (Run(TEXT("rev-parse --show-toplevel"), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), Out) == 0)
    {
        return Out.TrimStartAndEnd();
    }
    return FString();
}

bool FKeystoneGit::CommitExports(const FString& Message, FString& OutError)
{
    const FString Root = RepoRoot();
    if (Root.IsEmpty())
    {
        OutError = TEXT("This project isn't in a git working tree — commit BlueprintGraphs/ with your usual tool.");
        return false;
    }

    // Path to the export folder, relative to the repo root (project may be a subdir of the repo).
    FString ExportAbs = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), FKeystoneBlueprintExporter::ExportSubdir()));
    FString ExportRel = ExportAbs;
    FPaths::MakePathRelativeTo(ExportRel, *(Root / TEXT("")));

    FString Out;
    if (Run(FString::Printf(TEXT("add -- \"%s\""), *ExportRel), Root, Out) != 0)
    {
        OutError = FString::Printf(TEXT("git add failed: %s"), *Out.Left(800));
        return false;
    }
    const int32 CommitCode = Run(FString::Printf(TEXT("commit -m \"%s\""), *Message), Root, Out);
    if (CommitCode != 0)
    {
        if (Out.ToLower().Contains(TEXT("nothing to commit")))
        {
            OutError = TEXT("Nothing new to commit — Blueprint graphs are already up to date.");
            return false;
        }
        OutError = FString::Printf(TEXT("git commit failed: %s"), *Out.Left(800));
        return false;
    }
    if (Run(TEXT("push"), Root, Out) != 0)
    {
        OutError = FString::Printf(TEXT("git push failed: %s"), *Out.Left(800));
        return false;
    }
    return true;
}
