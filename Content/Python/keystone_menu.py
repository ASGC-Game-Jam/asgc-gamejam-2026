"""
Keystone menu — the artist-facing buttons. Adds a **Keystone** menu to the
editor's main menu bar (registered automatically by init_unreal.py). Nothing here
needs the Python console; every action is a click with plain-language dialogs.

Menu items:
  • Sync Source Art to Keystone…    copy this project's imported source files
                                    into SourceArt/ (asks before copying)
  • Commit & Push Source Art…       git add/commit/push just SourceArt + manifest
  • Toggle Auto-Capture on Import   copy each new import's source automatically
"""

from __future__ import annotations

import os
import subprocess

import unreal

import keystone_source_sync as sync


# ── small dialog helpers ─────────────────────────────────────────────────────
def _info(message: str) -> None:
    unreal.EditorDialog.show_message("Keystone", message, unreal.AppMsgType.OK)


def _confirm(message: str) -> bool:
    r = unreal.EditorDialog.show_message("Keystone", message, unreal.AppMsgType.YES_NO)
    return r == unreal.AppReturnType.YES


# ── actions ──────────────────────────────────────────────────────────────────
def sync_clicked() -> None:
    """Dry-run first (counts only), confirm, then copy for real — all via dialogs."""
    pre = sync.backfill(dry_run=True)
    todo = pre.get("would", 0)
    already = pre.get("already", 0)
    missing = pre.get("missing", 0)
    relocated = pre.get("relocated", 0)

    if todo == 0:
        msg = "Source art is already up to date in SourceArt/." if already else \
              "No imported source files were found in this project."
        if missing:
            msg += f"\n\n{missing} source file(s) couldn't be found on this machine " \
                   "(imported elsewhere). Run this on the PC that imported them to catch those."
        _info(msg)
        return

    msg = f"Found {todo} source file(s) to copy into SourceArt/."
    if relocated:
        msg += f"\n{relocated} found on another drive/path than recorded (will still copy)."
    if already:
        msg += f"\n{already} already current (will skip)."
    if missing:
        msg += f"\n{missing} not on this machine (can't copy)."
    msg += "\n\nCopy them now?"
    if not _confirm(msg):
        return

    res = sync.backfill(dry_run=False)
    copied = res.get("copied", 0)
    done = f"Copied {copied} source file(s) into SourceArt/ and updated the manifest."
    done += "\n\nNext: Keystone ▸ Commit & Push Source Art (or commit with your usual git tool)."
    _info(done)


def commit_clicked() -> None:
    """Stage ONLY SourceArt + .gitattributes, commit, and push — nothing else in the tree."""
    root = _git_root()
    if not root:
        _info("Couldn't find a git repository for this project. Commit SourceArt/ with your usual git tool.")
        return
    if not _have_git():
        _info("Git isn't available on this machine's PATH.\nCommit SourceArt/ with your usual git tool (e.g. GitHub Desktop, Fork).")
        return
    if not _confirm("Commit and push SourceArt/ + .gitattributes to the remote?\n\n"
                    "(Only those paths are staged — your other changes are untouched.)"):
        return

    steps = [
        (["git", "add", "SourceArt", ".gitattributes"], "stage"),
        (["git", "commit", "-m", "Add source art for Keystone (automated)"], "commit"),
        (["git", "push"], "push"),
    ]
    for cmd, label in steps:
        rc, out = _run(cmd, root)
        if rc != 0:
            if label == "commit" and "nothing to commit" in out.lower():
                _info("Nothing new to commit — SourceArt/ is already committed.")
                return
            _info(f"Git {label} failed:\n\n{out.strip()[:800]}\n\n"
                  "You can finish in your usual git tool.")
            return
    _info("SourceArt/ committed and pushed. Keystone will pick it up on its next sync.")


def toggle_auto_capture() -> None:
    if sync.is_hook_registered():
        sync.unregister_import_hook()
        _info("Auto-capture OFF — new imports won't copy their source automatically.")
    else:
        sync.register_import_hook()
        _info("Auto-capture ON — source art for each new import is copied automatically.\n\n"
              "(Lasts until you close the editor. Ask your TA to enable it permanently.)")


# ── git plumbing ─────────────────────────────────────────────────────────────
def _have_git() -> bool:
    try:
        return _run(["git", "--version"], os.getcwd())[0] == 0
    except Exception:
        return False


def _git_root() -> str | None:
    start = sync._project_dir()
    rc, out = _run(["git", "rev-parse", "--show-toplevel"], start)
    if rc == 0 and out.strip():
        return out.strip()
    # fall back: walk up looking for a .git dir
    d = start
    while True:
        if os.path.isdir(os.path.join(d, ".git")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            return None
        d = parent


def _run(cmd: list[str], cwd: str) -> tuple[int, str]:
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=300)
        return p.returncode, (p.stdout or "") + (p.stderr or "")
    except Exception as exc:
        return 1, str(exc)


# ── menu registration ────────────────────────────────────────────────────────
def register_menu() -> None:
    """Add the Keystone menu to the main menu bar (idempotent)."""
    try:
        menus = unreal.ToolMenus.get()
        main_menu = menus.find_menu("LevelEditor.MainMenu")
        if not main_menu:
            return
        ks = main_menu.add_sub_menu("LevelEditor.MainMenu", "", "Keystone", "Keystone")

        def add(name: str, label: str, fn: str, tooltip: str) -> None:
            e = unreal.ToolMenuEntry(name=name, type=unreal.MultiBlockType.MENU_ENTRY)
            e.set_label(label)
            e.set_tool_tip(tooltip)
            e.set_string_command(
                unreal.ToolMenuStringCommandType.PYTHON, "",
                f"import keystone_menu; keystone_menu.{fn}()",
            )
            ks.add_menu_entry("Keystone", e)

        add("SyncSourceArt", "Sync Source Art to Keystone…", "sync_clicked",
            "Copy this project's imported source files into SourceArt/")
        add("CommitSourceArt", "Commit & Push Source Art…", "commit_clicked",
            "Stage, commit and push SourceArt/ + .gitattributes")
        add("ToggleAutoCapture", "Toggle Auto-Capture on Import", "toggle_auto_capture",
            "Automatically copy each new import's source art")
        menus.refresh_all_widgets()
        unreal.log("[keystone] menu registered")
    except Exception as exc:
        unreal.log_warning(f"[keystone] could not register menu: {exc}")
