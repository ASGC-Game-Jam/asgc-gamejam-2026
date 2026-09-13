// Keystone Blueprint Exporter — the core, shared by every trigger (menu button, on-save
// hook, and the headless commandlet). Turns a UBlueprint's live graphs into the deterministic
// `.bpgraph.json` that Keystone reads to render a visual MR diff. No UI, no git here — just
// "Blueprint in, JSON file(s) out" so it's trivial to call from anywhere and reason about.
#pragma once

#include "CoreMinimal.h"

class UBlueprint;

/** Counts returned from a sweep, so callers (menu/commandlet) can report what happened. */
struct FKeystoneExportResult
{
    int32 Scanned = 0;   // Blueprints examined
    int32 Written = 0;   // .bpgraph.json files created or updated
    int32 Unchanged = 0; // already up to date (byte-identical) — skipped
    int32 Failed = 0;    // load/serialize errors
    int32 Pruned = 0;    // stale exports deleted (their Blueprint was renamed or removed)
    FString ManifestPath; // repo-relative path of the manifest written, or empty
};

class FKeystoneBlueprintExporter
{
public:
    /** Project-relative folder the exports are written under (sibling of Content/). */
    static const TCHAR* ExportSubdir() { return TEXT("BlueprintGraphs"); }

    /** Serialize one Blueprint's every graph to the `.bpgraph.json` shape. Pure — returns the
     *  JSON string; does not touch disk. Deterministic ordering (graphs/nodes/pins sorted by a
     *  stable key) so re-exports produce byte-identical output and git diffs stay minimal. */
    static FString BuildJson(UBlueprint* Blueprint);

    /** Absolute path the export for `Blueprint` is written to, e.g.
     *  `<Project>/BlueprintGraphs/Characters/BP_Hero.bpgraph.json`. Mirrors the package path
     *  under /Game so a .uasset maps to its export by a deterministic rule (plus the manifest). */
    static FString ExportFileFor(UBlueprint* Blueprint);

    /** Same path rule as `ExportFileFor`, but from a package name alone — the delete and rename
     *  hooks need it after the Blueprint is already gone. */
    static FString ExportFileForPackage(const FString& PackageName);

    /** Whether a package still backs a live export. Registry-based, because the disk is wrong at
     *  exactly the moments that matter: a deleted asset's .uasset is still on disk when
     *  OnAssetRemoved fires, and a rename usually leaves an ObjectRedirector file at the old path.
     *  `Unknown` means the file exists but the registry cannot account for it (an unmount, or a
     *  scan still running) — callers must treat it as "keep", never as "delete". */
    enum class EPackageState : uint8 { Live, Stale, Unknown };
    static EPackageState ClassifyPackage(const FString& PackageName);

    /** Delete the export file for a package, if one exists. Unconditional — decide with
     *  ClassifyPackage first. Returns true only if a file was actually removed. */
    static bool DeleteExportForPackage(const FString& PackageName);

    /** Delete every `.bpgraph.json` under the export folder that no live Blueprint maps to.
     *  `ExpectedFiles` is the set of absolute paths a sweep just wrote. Returns how many went. */
    static int32 PruneOrphanExports(const TSet<FString>& ExpectedFiles);

    /** Export a single Blueprint to disk (only rewrites if the JSON actually changed). Updates
     *  `InOutResult`. Used by the on-save hook for one asset. */
    static bool ExportOne(UBlueprint* Blueprint, FKeystoneExportResult& InOutResult);

    /** Sweep every Blueprint under `RootPath` (default /Game), export each, and (re)write the
     *  manifest that lists them. Used by the menu backfill and the commandlet. */
    static FKeystoneExportResult ExportAll(const FString& RootPath = TEXT("/Game"));
};
