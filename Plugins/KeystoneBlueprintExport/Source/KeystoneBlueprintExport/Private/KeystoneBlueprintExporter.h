// Keystone Blueprint Exporter — the core, shared by every trigger (menu button, on-save
// hook, and the headless commandlet). Turns an asset into the deterministic `.bpgraph.json`
// that Keystone reads to render a visual MR diff. No UI, no git here — just "asset in, JSON
// file(s) out" so it's trivial to call from anywhere and reason about.
//
// Every asset under /Game gets an export (format version 2). What goes in it depends on kind:
//   • Graph-bearing assets also carry their node graphs:
//       – Blueprints (incl. Anim/Widget BPs, Level Blueprints) — UbergraphPages/Functions/Macros/Delegates
//       – Materials + Material Functions (+Layers)             — the material editor's node graph, rebuilt transiently
//       – Niagara Systems, Emitters and Scripts                — every saved UEdGraph inside the package
//   • Everything carries `properties`: a reflection dump of the asset's editable data —
//     enumerators for an Enum, fields + defaults for a Struct, rows for a Data Table,
//     variables/components/class defaults for a Blueprint, and the plain property values of
//     anything else (an Input Action's triggers, a Texture's compression settings, …).
//   The bulk payload (pixels, vertices, samples) is never exported — only what the Details
//   panel shows — so a changed Texture still diffs as "settings changed", not as its image.
//
// Not exported: One-File-Per-Actor packages (`__ExternalActors__` / `__ExternalObjects__`) —
// one tiny file per placed actor; Keystone names the owning level instead.
#pragma once

#include "CoreMinimal.h"

class UObject;

/** On-disk format version written into every export and the manifest. 2 = graphs + properties. */
#define KEYSTONE_EXPORT_VERSION 2

/** Counts returned from a sweep, so callers (menu/commandlet) can report what happened. */
struct FKeystoneExportResult
{
    int32 Scanned = 0;   // assets examined
    int32 Written = 0;   // .bpgraph.json files created or updated
    int32 Unchanged = 0; // already up to date (byte-identical) — skipped
    int32 Failed = 0;    // load/serialize errors
    int32 Pruned = 0;    // stale exports deleted (their asset was renamed or removed)
    FString ManifestPath; // repo-relative path of the manifest written, or empty
};

class FKeystoneBlueprintExporter
{
public:
    /** Project-relative folder the exports are written under (sibling of Content/). */
    static const TCHAR* ExportSubdir() { return TEXT("BlueprintGraphs"); }

    /** True if `Asset` is something this exporter writes a file for: any real asset object
     *  (not a package, redirector or transient object) outside the One-File-Per-Actor folders. */
    static bool CanExport(const UObject* Asset);

    /** True for a package this exporter deliberately skips (external actors/objects). */
    static bool IsExcludedPackage(const FString& PackageName);

    /** Serialize one asset to the `.bpgraph.json` shape: its graphs (if it has any) and its
     *  properties. Pure — returns the JSON string; does not touch disk. Deterministic ordering
     *  (graphs/nodes/pins sorted by a stable key, properties in reflection order) so re-exports
     *  produce byte-identical output and git diffs stay minimal. */
    static FString BuildJson(UObject* Asset);

    /** Absolute path the export for `Asset` is written to, e.g.
     *  `<Project>/BlueprintGraphs/Characters/BP_Hero.bpgraph.json`. Mirrors the package path
     *  under /Game so a .uasset maps to its export by a deterministic rule (plus the manifest). */
    static FString ExportFileFor(UObject* Asset);

    /** Same path rule as `ExportFileFor`, but from a package name alone — the delete and rename
     *  hooks need it after the asset is already gone. */
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

    /** Delete every `.bpgraph.json` under the export folder that no live asset maps to.
     *  `ExpectedFiles` is the set of absolute paths a sweep just wrote. Returns how many went. */
    static int32 PruneOrphanExports(const TSet<FString>& ExpectedFiles);

    /** Export a single asset to disk (only rewrites if the JSON actually changed). Updates
     *  `InOutResult`. Used by the on-save hook for one asset. */
    static bool ExportOne(UObject* Asset, FKeystoneExportResult& InOutResult);

    /** Sweep every asset under `RootPath` (default /Game), export each, and (re)write the
     *  manifest that lists them. Used by the menu backfill and the commandlet. */
    static FKeystoneExportResult ExportAll(const FString& RootPath = TEXT("/Game"));
};
