"""
Keystone Source Sync — engine.

Copies Unreal asset *source art* (the original .png/.wav/.fbx an asset was
imported from) into a committed, LFS-tracked `SourceArt/` folder, and writes an
MD5 manifest that links each copy back to its .uasset in Keystone.

Artists don't call this directly — they click the **Keystone** menu (see
keystone_menu.py). It's also usable from the Output Log Python console:

    import keystone_source_sync as k
    k.backfill()                 # dry run — reports, copies nothing
    k.backfill(dry_run=False)    # copy + manifest + .gitattributes

`backfill()` returns a summary dict so the UI can show counts in a dialog.

Multi-drive note: Unreal records a source path *relative* to the .uproject when
the file is on the same drive, but stores an *absolute* path when it's on another
drive (a relative path can't cross drive letters). If that art later moves to a
different drive letter — or the record came from a teammate's machine — the literal
path no longer resolves. `_relocate()` re-finds those by matching the longest
trailing path segment under the project, its parents, every local drive root, and
any roots listed in the `KEYSTONE_SOURCE_ROOTS` env var (os.pathsep-separated).
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil

import unreal

# ── configuration ────────────────────────────────────────────────────────────
DEST_SUBDIR = "SourceArt"
MANIFEST_NAME = "keystone-source-manifest.json"
SOURCE_EXTS = {
    "fbx", "obj", "abc", "gltf", "glb", "dae", "usd", "usda", "usdc",
    "psd", "png", "tga", "jpg", "jpeg", "bmp", "tif", "tiff", "exr", "hdr",
    "wav", "mp3", "aif", "aiff", "ogg", "flac",
}


# ── path helpers ─────────────────────────────────────────────────────────────
def _project_dir() -> str:
    return os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


def _content_dir() -> str:
    return os.path.normpath(
        unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
    )


def _dest_root() -> str:
    return os.path.join(_project_dir(), DEST_SUBDIR)


def _ext_of(path: str) -> str:
    base = os.path.basename(path)
    dot = base.rfind(".")
    return base[dot + 1:].lower() if dot > 0 else ""


def _local_drive_roots() -> list[str]:
    """Every mounted drive root to search for relocated source art (Windows: A:\\..Z:\\)."""
    if os.name != "nt":
        return ["/"]
    return [f"{c}:\\" for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ" if os.path.exists(f"{c}:\\")]


def _extra_search_roots() -> list[str]:
    """Explicit roots from KEYSTONE_SOURCE_ROOTS (os.pathsep-separated), for art on a
    share or a tree no automatic root would reach."""
    raw = os.environ.get("KEYSTONE_SOURCE_ROOTS", "")
    return [p for p in (seg.strip() for seg in raw.split(os.pathsep)) if p and os.path.isdir(p)]


def _relocate(path: str) -> str | None:
    """The recorded path didn't resolve where UE wrote it — usually because the art now
    lives on a different drive letter, or the record came from another machine. Re-find
    it by matching the *longest trailing run of path segments* under the project, its
    parents, any KEYSTONE_SOURCE_ROOTS, and every local drive root. Requires at least
    dir + filename to agree so a bare basename can't grab an unrelated same-named file.
    Longest-suffix-wins, so the most specific tail match is preferred over a shallow one."""
    norm = path.replace("\\", "/").lstrip("/")
    if len(norm) >= 2 and norm[1] == ":":  # drop a leading drive spec ("E:")
        norm = norm[2:].lstrip("/")
    segments = [s for s in norm.split("/") if s and s != ".."]
    if len(segments) < 2:
        return None
    project = _project_dir()
    roots = [
        _content_dir(), project, os.path.dirname(project), os.path.dirname(os.path.dirname(project)),
        *_extra_search_roots(), *_local_drive_roots(),
    ]
    tried: set[str] = set()
    for take in range(len(segments), 1, -1):  # longest suffix first = most specific
        tail = os.path.join(*segments[-take:])
        for base in roots:
            if not base:
                continue
            cand = os.path.normpath(os.path.join(base, tail))
            if cand in tried:
                continue
            tried.add(cand)
            if os.path.isfile(cand):
                return cand
    return None


def _resolve_source(path: str) -> tuple[str | None, str]:
    """Recorded import path → (absolute file on this machine, how) where `how` is
    'direct' (found where recorded), 'relocated' (found elsewhere on this machine),
    or 'missing' (genuinely not here)."""
    path = path.strip().replace("\\", "/")
    if not path:
        return None, "missing"
    if os.path.isabs(path) and os.path.isfile(path):
        return os.path.normpath(path), "direct"
    project = _project_dir()
    for base in (_content_dir(), project, os.path.dirname(project), os.path.dirname(os.path.dirname(project))):
        cand = os.path.normpath(os.path.join(base, path))
        if os.path.isfile(cand):
            return cand, "direct"
    relocated = _relocate(path)
    if relocated:
        return relocated, "relocated"
    return None, "missing"


def _package_path(asset_path: str) -> str:
    return asset_path.split(".")[0]


def _md5(path: str) -> str:
    h = hashlib.md5()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# ── discover + copy one asset's sources ──────────────────────────────────────
def _import_filenames(asset) -> list[str]:
    try:
        import_data = asset.get_editor_property("asset_import_data")
    except Exception:
        return []
    if not import_data:
        return []
    try:
        return list(import_data.extract_filenames())
    except Exception:
        return []


def _copy_for_asset(asset_path: str, asset, dry_run: bool, counts: dict) -> list[dict]:
    package = _package_path(asset_path)
    rel_pkg = package[len("/Game/"):] if package.startswith("/Game/") else package.lstrip("/")
    cls = asset.get_class().get_name() if asset else "?"
    entries: list[dict] = []

    for recorded in _import_filenames(asset):
        if _ext_of(recorded) not in SOURCE_EXTS:
            continue
        counts["refs"] += 1
        filename = os.path.basename(recorded.replace("\\", "/"))
        resolved, how = _resolve_source(recorded)
        if not resolved:
            counts["missing"] += 1
            unreal.log(f"  MISSING  {package}  <-  {recorded}  (not on this machine)")
            continue
        if how == "relocated":
            counts["relocated"] = counts.get("relocated", 0) + 1
            unreal.log(f"  relocated {package}  <-  {recorded}  =>  {resolved}")

        dest_dir = os.path.join(_dest_root(), rel_pkg)
        dest = os.path.join(dest_dir, filename)
        digest = _md5(resolved)
        size = os.path.getsize(resolved)
        rel_dest = os.path.relpath(dest, _project_dir()).replace("\\", "/")

        if os.path.isfile(dest) and _md5(dest) == digest:
            counts["already"] += 1
        elif dry_run:
            counts["would"] += 1
        else:
            os.makedirs(dest_dir, exist_ok=True)
            shutil.copy2(resolved, dest)
            counts["copied"] += 1
            unreal.log(f"  copied   {package}  <-  {filename}  -> {rel_dest}")

        entries.append({
            "package": package,
            "asset": os.path.basename(package),
            "class": cls,
            "source": {
                "filename": filename,
                "originalPath": recorded.replace("\\", "/"),
                "md5": digest,
                "sizeBytes": size,
            },
            "copiedTo": rel_dest,
        })
    return entries


# ── manifest + LFS ───────────────────────────────────────────────────────────
def _write_manifest(entries: list[dict]) -> str:
    os.makedirs(_dest_root(), exist_ok=True)
    path = os.path.join(_dest_root(), MANIFEST_NAME)
    entries.sort(key=lambda e: e["package"])
    with open(path, "w", encoding="utf-8") as fh:
        json.dump({
            "version": 1,
            "generatedBy": "keystone_source_sync",
            "project": os.path.basename(_project_dir().rstrip("/\\")),
            "count": len(entries),
            "entries": entries,
        }, fh, indent=2)
    return path


def _ensure_gitattributes(exts: set[str]) -> None:
    ga = os.path.join(_project_dir(), ".gitattributes")
    existing = ""
    if os.path.isfile(ga):
        with open(ga, "r", encoding="utf-8") as fh:
            existing = fh.read()
    lines = []
    for ext in sorted(exts):
        pattern = f"{DEST_SUBDIR}/**/*.{ext}"
        if pattern not in existing:
            lines.append(f"{pattern} filter=lfs diff=lfs merge=lfs -text")
    if lines:
        with open(ga, "a", encoding="utf-8") as fh:
            fh.write("\n# Keystone source-art (LFS)\n" + "\n".join(lines) + "\n")


# ── public entry point ───────────────────────────────────────────────────────
def backfill(dry_run: bool = True, root: str = "/Game") -> dict:
    """Sweep `root`, copy still-present source art, write the manifest. Returns a
    summary dict: {scanned, refs, copied, would, already, missing, manifest}."""
    unreal.log(f"[keystone] scanning {root} (dry_run={dry_run})")
    asset_paths = unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False)
    counts = {"scanned": 0, "refs": 0, "copied": 0, "would": 0, "already": 0, "missing": 0, "relocated": 0}
    entries: list[dict] = []

    with unreal.ScopedSlowTask(len(asset_paths), "Keystone: copying source art") as task:
        task.make_dialog(True)
        for ap in asset_paths:
            if task.should_cancel():
                counts["cancelled"] = True
                break
            task.enter_progress_frame(1)
            counts["scanned"] += 1
            try:
                asset = unreal.EditorAssetLibrary.load_asset(ap)
            except Exception:
                continue
            if asset:
                entries.extend(_copy_for_asset(ap, asset, dry_run, counts))

    counts["manifest"] = None
    if not dry_run and entries:
        counts["manifest"] = _write_manifest(entries)
        _ensure_gitattributes({_ext_of(e["source"]["filename"]) for e in entries})

    unreal.log(f"[keystone] done: {counts}")
    return counts


# ── live import hook ─────────────────────────────────────────────────────────
_HOOK_REGISTERED = False


def _on_post_import(factory, created_object) -> None:
    if not created_object:
        return
    try:
        counts = {"refs": 0, "copied": 0, "would": 0, "already": 0, "missing": 0, "relocated": 0}
        entries = _copy_for_asset(created_object.get_path_name(), created_object, False, counts)
        if entries:
            _merge_into_manifest(entries)
    except Exception as exc:
        unreal.log_warning(f"[keystone] import hook error: {exc}")


def _merge_into_manifest(new_entries: list[dict]) -> None:
    path = os.path.join(_dest_root(), MANIFEST_NAME)
    existing: list[dict] = []
    if os.path.isfile(path):
        try:
            with open(path, "r", encoding="utf-8") as fh:
                existing = json.load(fh).get("entries", [])
        except Exception:
            existing = []
    by_key = {(e["package"], e["source"]["filename"]): e for e in existing}
    for e in new_entries:
        by_key[(e["package"], e["source"]["filename"])] = e
    _write_manifest(list(by_key.values()))
    _ensure_gitattributes({_ext_of(e["source"]["filename"]) for e in new_entries})


def is_hook_registered() -> bool:
    return _HOOK_REGISTERED


def register_import_hook() -> None:
    global _HOOK_REGISTERED
    if _HOOK_REGISTERED:
        return
    try:
        subsystem = unreal.get_editor_subsystem(unreal.ImportSubsystem)
    except Exception:
        subsystem = unreal.import_subsystem()
    subsystem.on_asset_post_import.add_callable(_on_post_import)
    _HOOK_REGISTERED = True
    unreal.log("[keystone] import hook registered")


def unregister_import_hook() -> None:
    global _HOOK_REGISTERED
    if not _HOOK_REGISTERED:
        return
    try:
        subsystem = unreal.get_editor_subsystem(unreal.ImportSubsystem)
    except Exception:
        subsystem = unreal.import_subsystem()
    subsystem.on_asset_post_import.remove_callable(_on_post_import)
    _HOOK_REGISTERED = False
    unreal.log("[keystone] import hook removed")
