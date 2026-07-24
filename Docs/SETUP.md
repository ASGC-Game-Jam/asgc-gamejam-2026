# Development Setup

This guide walks through setting up your development environment for the
ASGC Game Jam 2026 project

## Requirements

Install the following before cloning the repository

- Git
- Git LFS
- Unreal Engine 5.7.4
- Visual Studio 2022 (or another IDE configured for Unreal Engine C++ development)
  - MSVC
  - Windows SDK
  - C++ profiling tools (recommended)

Verify Git LFS is installed

```bash
git lfs version
```

If needed, initialize Git LFS

```bash
git lfs install
```

## Clone the Repository

Clone the repository

```bash
git clone https//github.com/ASGC-Game-Jam/asgc-gamejam-2026.git
cd asgc-gamejam-2026
```

Git LFS should automatically download the project assets
If it doesn't, run

```bash
git lfs pull
```

If Git prompts for Git LFS credentials

1. Log into Gitea using **Sign in with GitHub**
2. Go to **Settings → Account**
3. Set a local Gitea password
4. Retry using
   - **Username** Your Gitea username (typically the same as your GitHub username)
   - **Password** Your Gitea password

> **Note**
>
> We have not been able to consistently reproduce this issue, but some users have
> reported that Git LFS authentication fails until a local Gitea password has
> been configured

## Enable Git LFS Lock Verification

Enable lock verification for this repository

```bash
git config lfs.locksverify true
```

Verify it is enabled

```bash
git config --get lfs.locksverify
```

Expected output

```text
true
```

## Open the Project

Open `ProjectAtlantis.uproject`.

If Unreal prompts you to build the project files, select **Yes**

The first launch may take several minutes while Unreal generates project files,
compiles shaders, and prepares the project

## Verify Your Setup

Before making changes, verify that

- The project opens without errors
- The project compiles successfully
- You can launch the editor
- You can load the default map
- Git LFS assets downloaded successfully

If you plan to edit shared binary assets, verify the lock server is reachable

```bash
git lfs locks
```

If anything fails, ask for help before continuing

## Next Steps

Once your environment is working

- Read `CONTRIBUTING.md`
- Pull the latest changes from `main`
- Create a feature branch
- Lock shared binary assets before editing them
- Start building
