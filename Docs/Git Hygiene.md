# Git Hygiene

This guide covers the Git habits expected for contributors to the ASGC Game Jam 2026 repository.
It is not meant to teach every Git command or enforce one "correct" way to use Git.
The goal is to keep work understandable, reviewable, and safe to integrate across a large asynchronous team

For initial setup, cloning, and project requirements, see `Docs/SETUP.md`

For the general contribution workflow, see `CONTRIBUTING.md`

---

## Core Rules

1) **Never work directly on `main`**
2) **Never force-push `main`** except during an explicitly coordinated repository recovery
3) Work from a GitHub Issue and use a short-lived branch for your task
4) Keep changes focused on the task you are working on
5) Lock shared Unreal binary assets before editing them
6) Review what you changed before committing or opening a Pull Request
7) Test your work before requesting review
8) Use the Pull Request template
9) Clean up the PR title and description before it is merged
10) If you are unsure about a destructive Git operation or conflict, ask before continuing

---

# 1) Start From an Issue

Work should normally correspond to a GitHub Issue

Branch names use the Issue number followed by a short description

```text
87-audio-footstep-system
104-gameplay-interaction-update
117-ai-enemy-state-tree
```

Do not include your name in the branch

Task ownership is already represented in GitHub Projects, and another contributor may need to take over the work later

---

# 2) Start From `main`

Before creating a task branch, make sure your local `main` is current

```bash
git switch main
git pull
git switch -c 87-footstep-system
```

Do not reuse an old merged branch for unrelated work

---

# 3) Keep Branches Short-Lived

`main` is the project's primary integration branch

Most work should follow this flow

```text
Issue
  ↓
Task Branch
  ↓
Pull Request
  ↓
main
```

A temporary feature/integration branch may be used when several related tasks must work together before the overall feature can be validated

```
87-audio-footstep-system ─────────────┐
104-gameplay-interaction-update ──────┼──> 14-asset-pipeline──> main
117-ai-enemy-state-tree ──────────────┘
```

Do not create a long-lived feature or integration branch just because an Issue is classified as a Feature

If individual tasks can be completed and validated independently, they should normally merge directly into `main`

The project intentionally does **NOT** use a permanent `develop` branch

---

# 4) You Do Not Need to Sync Constantly

A change landing on `main` does not automatically mean your branch needs to be updated

Sync when

* a change on `main` affects the files or systems you are working on
* GitHub reports that your branch can't be merged cleanly
* you need newer work from `main`
* your Lead or reviewer asks you to update your branch

If someone adds unrelated documentation while you are working on gameplay code, there is usually no reason to immediately update your branch

Both rebasing and merging `main` into a task branch can be appropriate

Experienced contributors and Feature Owners can choose the approach that best fits the situation

If you do not understand the difference, ask rather than experimenting on important work

---

# 5) Check Before You Commit

Before committing, inspect your working tree

```bash
git status
```

Review your changes

```bash
git diff
```

Make sure you understand why every changed file belongs in your task

Watch for

* unrelated changes
* accidentally moved assets
* generated Unreal files
* editor configuration
* temporary files
* files modified only because they were opened
* changes belonging to another task

Do not use

```bash
git add .
```

without first checking what it will stage

For a large Unreal project, accidentally committing unrelated files can create unnecessary conflicts for other contributors

---

# 6) Keep Commits Understandable

You do not need to maintain a perfect commit history while working

Make commits whenever they help you save or organize your work

Use meaningful messages where practical

The project prefers Conventional Commit prefixes

```text
feat: add footstep audio system
fix: correct invalid asset reference
docs: clarify Git LFS setup
refactor: reorganize inventory initialization
chore: update project configuration
```

Common prefixes include

```text
feat
fix
docs
refactor
chore
```

Avoid messages such as:

```text
stuff
changes
fix
test
asdf
final
final2
```

Ordinary Pull Requests are squash-merged, so temporary development commits do not need to be individually perfect. 

---

# 7) Unreal Assets and Git LFS

Unreal assets such as `.uasset` and `.umap` are binary files

Unlike normal source files, two versions of these assets generally can't be meaningfully merged

That makes locking important

Before editing a shared binary asset, check existing locks

```bash
git lfs locks
```

Lock the asset

```bash
git lfs lock Content/Path/Asset.uasset
```

When you are finished with the asset

```bash
git lfs unlock Content/Path/Asset.uasset
```

Do not leave assets locked when you are no longer actively working on them

If an asset you need is already locked, contact the person holding the lock rather than editing a competing copy

Git LFS handles storage and transfer of these assets, but **locking is an explicit contributor responsibility**

Do not modify

```text
.gitattributes
.lfsconfig
```

or the repository's LFS configuration unless a Tech Lead has specifically asked you to

---

# 8) Conflicts

Normal text conflicts can often be resolved safely if you understand both changes

Use additional caution with

* `.uasset`
* `.umap`
* Unreal project configuration
* build configuration
* repository infrastructure
* files changed by several contributors at once

Do not blindly choose `ours` or `theirs` just to make Git stop reporting a conflict

A clean Git status does not mean the correct changes survived

If you do not understand a conflict, ask the relevant Senior, Lead, or Feature Owner

---

# 9) Before Opening a Pull Request

Before requesting review

* confirm that you are on the correct branch
* review `git status`
* review your final diff
* remove unrelated changes
* confirm Git LFS uploads completed
* build the project where applicable
* test the behavior you changed
* update relevant documentation
* link the appropriate Issue
* complete the Pull Request template

Do a self-review before asking someone else to review your work

A reviewer should not be the first person to inspect your complete diff

---

# 10) Keep the Pull Request Useful

The project's Git history is intentionally kept relatively clean

`main` is protected, and changes enter through Pull Requests with approval and CODEOWNERS review where applicable. Ordinary task PRs are generally squash-merged

This means the final PR metadata matters

Before marking a PR ready for review, make sure the

* **title** clearly describes the change
* **description** accurately describes what the branch now contains
* relevant Issue is linked
* testing performed is documented
* known limitations or follow-up work are mentioned

The PR does not need to remain perfectly documented during active development

It **does** need to accurately describe the final change before merge

GitHub Releases are currently being used instead of maintaining a separate `CHANGELOG.md`, which makes clean PR and commit information especially useful

---

# 11) Squash, Merge, and Rebase

For normal contributors, you generally do not need to manage the final integration strategy yourself

The common case is

```text
task branch ──(squash)──> main
```

Squashing allows contributors to make useful development commits without filling `main` with temporary commits such as typo fixes or intermediate experiments

For collaborative features, individual task branches may first be squash-merged into a temporary integration branch

```
87-audio-footstep-system ────────────(squash)─┐
104-gameplay-interaction-update ─────(squash)─┼──> 14-asset-pipeline─(rebase)─> main
117-ai-enemy-state-tree ─────────────(squash)─┘
```

The Feature Owner or reviewer can then determine the appropriate way to integrate that branch into `main`

Do not rewrite history simply because you believe a Git graph should look cleaner

Repository correctness and preserving other contributors' work are more important than visual cleanliness

---

# 12) After Your PR Is Merged

Once your work is merged

```bash
git switch main
git pull
```

Delete the completed local branch when you no longer need it

```bash
git branch -d 87-footstep-system
```

Also

* unlock any LFS assets you no longer need
* verify your Issue / task state is correct
* create a new branch for your next task

Do not continue unrelated work on a branch whose PR has already been merged

---

# 13) Ask Before Making It Worse

Git mistakes are usually recoverable

They become harder to recover from when someone repeatedly runs commands they do not understand in an attempt to fix the first problem

Stop and ask for help if

* you accidentally committed directly to `main`
* Git reports a conflict you do not understand
* you think you overwrote someone else's work
* an Unreal binary asset has competing changes
* Git LFS authentication or locking fails
* you committed files that should not be in the repository
* a rebase or merge produced unexpected results
* you believe you need `--force`
* files suddenly appear deleted or modified unexpectedly

In particular

> If your solution involves force-pushing something you did not intentionally create as your own task branch, ask first

`main` should never be force-pushed during ordinary development

---

# Quick Reference

Before starting

* [ ] Find or create the appropriate Issue
* [ ] Update local `main`
* [ ] Create a short-lived task branch
* [ ] Check and lock shared Unreal assets

Before committing

* [ ] Run `git status`
* [ ] Review what changed
* [ ] Stage only files belonging to your task
* [ ] Use a meaningful commit message

Before requesting review

* [ ] Test your changes
* [ ] Review the complete diff
* [ ] Remove unrelated files
* [ ] Verify LFS uploads
* [ ] Complete the PR template
* [ ] Clean up the PR title and description
* [ ] Link the appropriate Issue

After merge

* [ ] Return to `main` and pull
* [ ] Unlock assets
* [ ] Delete the completed task branch
* [ ] Start new work on a new branch

---

## The Short Version

If you remember nothing else

* Branch before working
* Lock before editing shared Unreal assets
* Check before committing
* Review before requesting review
* Unlock when finished

And if Git is doing something you do not understand, **ask before using a destructive command**
