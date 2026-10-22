# Project Borealis Git plugin trial

The project vendors Git LFS 2 version 3.16 from
https://github.com/ProjectBorealis/UEGitPlugin at commit
`30402ac4cdd9f8d9369c948ea4f3b9f5be9bc313` (upstream `dev`).
Source, icon, MIT license, upstream README, and the upstream Windows, Mac,
and Linux LFS helpers are included under `Plugins/GitSourceControl`.
This is a project plugin, so teammates receive it with the repository.
No engine installation changes or submodule setup are needed.

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

## Updating or removing

For an update, replace the vendored files from a reviewed upstream revision,
including its matching LFS helpers, and update this commit reference.
The helpers are intentional upstream dependencies, not local build outputs.

To stop testing, close Unreal and switch back to main. To remove the integration
from a merged branch, remove `Plugins/GitSourceControl`, its project enablement,
the required auto-add default, and its `.gitignore` exception.

## Installation validation

- Unreal 5.7.4 Development Editor / Win64 build succeeded, including
  UnrealEditor-GitSourceControl.dll.
- Bundled Windows LFS helper reports version 3.3.0.
- Integration files pass whitespace checks. Vendored source is unchanged
  from upstream, including its existing whitespace.
- Interactive editor connection, Gitea lock/unlock, and PIE remain manual tests.

