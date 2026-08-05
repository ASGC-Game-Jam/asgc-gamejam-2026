// Tiny git shell helper, shared by the menu button and the commandlet. Mirrors the source-art
// tool's plumbing (tools/unreal-source-sync/keystone_menu.py): stage ONLY the export folder,
// commit, push — never the artist's other changes. Best-effort; failures are reported, not fatal.
#pragma once

#include "CoreMinimal.h"

class FKeystoneGit
{
public:
    /** Run `git <Args>` in `WorkingDir`. Returns the exit code; combined stdout+stderr in Out. */
    static int32 Run(const FString& Args, const FString& WorkingDir, FString& Out);

    /** True if a git executable is reachable on PATH. */
    static bool Available();

    /** Repo root containing the project, or empty if this isn't a git working tree. */
    static FString RepoRoot();

    /** Stage just `BlueprintGraphs/`, commit with `Message`, and push. Returns false (with a
     *  human-readable reason in OutError) on any failure, including "nothing to commit". */
    static bool CommitExports(const FString& Message, FString& OutError);
};
