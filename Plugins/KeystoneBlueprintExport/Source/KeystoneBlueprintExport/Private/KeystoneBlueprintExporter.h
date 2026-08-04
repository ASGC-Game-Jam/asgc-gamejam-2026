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

    /** Export a single Blueprint to disk (only rewrites if the JSON actually changed). Updates
     *  `InOutResult`. Used by the on-save hook for one asset. */
    static bool ExportOne(UBlueprint* Blueprint, FKeystoneExportResult& InOutResult);

    /** Sweep every Blueprint under `RootPath` (default /Game), export each, and (re)write the
     *  manifest that lists them. Used by the menu backfill and the commandlet. */
    static FKeystoneExportResult ExportAll(const FString& RootPath = TEXT("/Game"));
};
