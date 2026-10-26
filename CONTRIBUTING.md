# Contributing

Thanks for contributing to the ASGC Game Jam 2026 project.

## Before You Start

- Complete the setup in [Docs/SETUP.md](./Docs/SETUP.md)
- Check the existing [Issues](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/issues) to see what needs to be done
- If you're unsure where to start, ask in Discord or open a Discussion

## Workflow

1. Sync your local repository
2. Create a new branch from `main`
3. Make your changes
4. Test your changes
5. Commit your work
6. Open a Pull Request

Please keep Pull Requests focused on a single feature or fix whenever possible

For more details refer to [Git Hygiene](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/Docs/Git%20Hygiene.md)

## Pull Requests

Before submitting a Pull Request, make sure:

- The project builds successfully
- Your changes have been tested
- Unrelated files are not included
- Any relevant documentation has been updated

A reviewer may request changes before the Pull Request is merged.

## Git LFS

This project uses [Git LFS](https://git-lfs.com/) for Unreal Engine assets.

If Git LFS is installed correctly, asset management should happen automatically. Do not remove or modify the repository's Git LFS configuration unless instructed by the Tech team.

## Locking Assets Before You Edit Them

Unreal assets are binary files. If two people edit the same `.uasset` at the same time, Git cannot merge the result — one person's work has to be thrown away and redone. Locking prevents that.

Connect **Git LFS 2** in Unreal, refresh asset
status, and use the Content Browser's **Revision Control > Check Out** before
editing. Confirm the lock owner in the asset tooltip. Multi-select supports
checking out several assets together. See the [plugin workflow](./Docs/GitWorkflow.md)
for connection steps, cleanup, and limitations. On branches without the plugin,
or for external source art, use `git lfs locks` and `git lfs lock "path/to/asset"`.

**Keep locks through PR merge.** Commit and push with Git or your Git client:
the plugin's **Submit** action commits, pushes, and releases locks immediately
after a successful push, which is too early for our normal review workflow.
After merge, release your locks with `git lfs unlock "path/to/asset"` and refresh
Unreal status. Unchanged or abandoned assets may be released once no pending
work needs preserving. Never force-unlock another contributor's work.

Keystone's **File Locks** view shows who holds each lock and how long they
have held it. Use `git lfs locks` to inspect server locks from the CLI.

The file types that must be locked are the ones marked `lockable` in [.gitattributes](./.gitattributes): `.uasset`, `.umap`, `.fbx`, `.3ds`, `.psd` and the other binary formats listed there. Text files (code, config, docs) are never locked — merge those normally.

### LFS Lock Validation

When you open a Pull Request, Keystone checks it: for every lockable file the Pull Request changes, do you hold the required lock?

- **Pass** — you held the locks. Nothing to do.
- **Not locked** — you edited a lockable file without locking it. Lock it now; Keystone shows the exact command. Locking after the fact still helps, because it tells everyone else the file is in play while your Pull Request is open.
- **Locked by someone else** — stop and talk to them before going further. Two people have edited the same binary file, and only one version can survive. The Keystone check names who holds it.

**A failing check blocks the merge in Keystone.** Leads can override it when there's a good reason (the person who holds the lock has finished, or has left the project), and every override is recorded.

If the check can't reach the LFS server it reports that instead, and doesn't block anything.

### If you forget

It happens. Lock the file, finish the change, and mention it on the Pull Request so a reviewer knows the collision was checked. If someone else was editing the same asset, sort out whose version to keep *before* merging — untangling it afterwards is much harder.

## Reporting Issues

If you find a bug:

- Search existing Issues first
- If one doesn't exist, create a new Issue with as much detail as possible (use the [Bug template](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/issues/new/choose))
- Include steps to reproduce the problem whenever possible

## Questions

If you're unsure about something, ask before spending time heading in the wrong direction. We'd rather answer questions early than untangle problems later.

## Code of Conduct

Treat other contributors with respect. We all have different levels of experience, and everyone is here to learn and build something together.
