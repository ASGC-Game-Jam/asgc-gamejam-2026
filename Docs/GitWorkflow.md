# Project Borealis Git workflow

The project vendors Git LFS 2 version 3.16 from
https://github.com/ProjectBorealis/UEGitPlugin at commit
`30402ac4cdd9f8d9369c948ea4f3b9f5be9bc313` (upstream `dev`).
Source, icon, MIT license, and upstream README are included under
`Plugins/GitSourceControl`. The project uses installed Git LFS rather than
upstream's bundled executables.
This is a project plugin, so teammates receive it with the repository.
No engine installation changes or submodule setup are needed.

Tracking issue: [#373](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/issues/373).

## Build and test

1. Close Unreal before switching to this branch. Pull LFS assets as usual.
2. Build `ProjectAtlantisEditor` in Development Editor / Win64. The checked-in
   project and setup guide currently select Unreal 5.7.4; use that engine.
3. Open `ProjectAtlantis.uproject`. In Plugins, verify **Git LFS 2** is enabled.
4. In Revision Control, select Git and connect to the existing repository.
   Enable the Git LFS 2 locking workflow and use your Gitea account's lock-owner
   name. Connection settings and credentials remain local to each developer.
5. Verify clean/modified asset status and lock-owner display. On a disposable
   test asset, check out, confirm its lock with `git lfs locks`, then revert
   and unlock it. Coordinate with teammates before locking shared assets.
6. Verify Play In Editor, save, and restart. Only test submit/push with changes
   you intend to publish: upstream's Submit operation commits, pushes, and
   releases locks.

The repository's `.lfsconfig` continues to route LFS and locks to Gitea while
Git source stays on GitHub. No LFS endpoint or authentication settings change.
New files must be added explicitly (`git add`); the plugin's required
`bSCCAutoAddNewFiles=False` default is included. Automatic checkout and status
branch integration are not enabled by this installation.

## Daily asset workflow

The project uses the Project Borealis provider for revision control in Unreal.
These instructions apply when **Git LFS 2** is enabled and connected; use the CLI
fallback below when the plugin is unavailable. GitHub still hosts Git and PRs; Gitea still hosts
LFS objects and locks, and Keystone still provides File Locks and PR validation.

1. Save your work and close Unreal before switching branches. Update from main
   and create your task branch using Git or your Git client, then reopen Unreal.
2. Connect **Revision Control > Git** with **Uses Git LFS 2 File Locking workflow**
   enabled. Set the lock-owner name to your Gitea identity, which may differ
   from Git's commit author. If your checkout appears owned by someone else,
   compare the displayed owner with this setting before editing.
3. Refresh asset status. In the Content Browser, select the assets you intend
   to edit and use **Revision Control > Check Out**. Multiple selection is
   supported; avoid checking out whole folders unless you need every asset.
4. Confirm checkout succeeded and inspect the asset tooltip for the owner.
   Your locks appear with a red check mark; other users' locks use blue.
   Stop and coordinate if another person holds the lock or the operation fails.
   Do not treat a writable local file or a stale icon as proof of ownership.
5. Edit, save, compile changed Blueprints, and test. Add new files explicitly
   with `git add`; editor auto-add is disabled. Review the complete Git diff,
   including assets indirectly modified by your edit, and lock those too.
6. Commit and push using Git or your Git client, then open a PR in GitHub.
   **Keep locks until merge** so Keystone can validate ownership and teammates
   do not begin competing edits while the PR is under review.
7. After merge, confirm your changes reached main and release your locks with
   the CLI below. Refresh editor status afterward. For unchanged assets, the
   plugin also exposes an unlock action. Revert abandoned changes only after
   confirming you no longer need them; revert discards local work.

### Submit releases locks before PR merge

Upstream **Submit to revision control** creates a commit, pushes to
`origin HEAD`, and releases locks after a successful push. It can also push
previous local commits, so reviewing only the selected assets is insufficient.
It does not wait for GitHub review or merge. For shared binary work, use the
external commit/push workflow above unless a Lead coordinates an exception.
If Submit was used accidentally, refresh the server's locks, coordinate any
competing owner, reacquire available locks immediately, and disclose the gap
on the PR. Do not force-unlock someone else's work.

### CLI fallback and source art

Use these commands from the repository root when the plugin is unavailable,
for source art edited outside Unreal, or for post-merge cleanup:

```bash
git lfs locks
git lfs lock "Content/ProjectAtlantis/Maps/L_Example.umap"
# Edit, test, commit, push, and merge the PR while retaining the lock.
git lfs unlock "Content/ProjectAtlantis/Maps/L_Example.umap"
```

Keystone's **File Locks** view also shows ownership. Lock all relevant formats
marked `lockable` in `.gitattributes`, not just maps and Unreal assets.
Unchanged assets may be released early after confirming there is no pending
work. If authentication fails, follow [SETUP.md](SETUP.md) and
[Gitea 2FA.md](Gitea%202FA.md); do not change `.lfsconfig` to bypass Gitea.

### Integration limitations

- Automatic checkout and status-branch integration are not enabled. Manually
  check out before editing; do not assume cross-branch stale-asset protection.
- Locks are repository-wide, not branch-specific. Track which branch holds
  your actual changes and coordinate handoffs until those changes reach main.
- Locking cannot merge competing binary edits or prevent every offline edit.
  Refresh status and verify server ownership when uncertain.
- Editor Pull operates on the current branch, not automatically on main.
  Save first; use Git for branch changes, rebases, and conflict resolution.
- Credentials remain local and both GitHub and Gitea access must work.
  Cached lock status is not a substitute for a successful server operation.

## Updating or removing

For an update, replace the vendored source from a reviewed upstream revision
and update the commit reference. Preserve these project adaptations:

- `PrivateDefinitions.Add("GIT_USE_CUSTOM_LFS=0")` in the module build rules.
- No runtime dependencies on bundled LFS executables.
- Standard LFS lock/unlock calls run once per selected path, aggregating failures
  and continuing through the selection. Upstream's custom executable supports
  batching; standard Git LFS takes one path per command.
- The `Atlantis.Git.SystemLfs` automation specs for the system-LFS path.

Do not reintroduce the upstream helper binaries when updating.

To remove the integration, remove `Plugins/GitSourceControl`, its project enablement,
and the required auto-add default.

## Installed Git LFS

Install Git LFS on every workstation and verify `git lfs version` using the same
Git executable configured in Unreal. The plugin executes that Git with `lfs`
commands; it does not download or ship another copy. Restart Unreal after
installing Git/LFS or changing PATH. On macOS/Linux, ensure the GUI-launched
editor can find LFS as well as your interactive shell.

Upstream explicitly supports this mode in its [3.10 release notes](https://github.com/ProjectBorealis/UEGitPlugin/releases/tag/3.10).
Its custom LFS adds lock/unlock performance optimizations; selecting many assets
may be slower with installed LFS. Editor checkout, ownership, caching, and the
existing Gitea endpoint still apply. Missing LFS or failed authentication must
be fixed before editing shared assets.

## System-LFS validation (2026-10-28)

Run `Automation RunTests Atlantis.Git.SystemLfs` in the editor or via
`UnrealEditor-Cmd.exe`. To include the live server test, launch with
`-GitLfsServerTests`; it uses existing credentials and creates temporary unique
locks under `Saved/GitLfsIntegration/`. Without that flag, only the local version
and error-propagation checks run. Never interrupt a live lock test without
checking for leftover locks under that test prefix.
