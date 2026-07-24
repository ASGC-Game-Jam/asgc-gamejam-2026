# Development Setup

This guide walks through setting up your development environment for the
ASGC Game Jam 2026 project.

## Requirements

Install the following before cloning the repository

-   Git
-   Git LFS
-   Unreal Engine 5.7.4
-   Visual Studio, Rider or another IDE configured for Game Development
    with C++# Development Setup

This guide walks through setting up your development environment for the
ASGC Game Jam 2026 project.

## Requirements

Install the following before cloning the repository

- Git
- Git LFS
- Unreal Engine 5.7.4
- Visual Studio 2022 or another IDE configured for Game Development with C++
  - MSVC
  - Windows SDK
  - C++ profiling tools (recommended)

Verify that Git LFS is installed

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
git clone https://github.com/ASGC-Game-Jam/asgc-gamejam-2026.git
cd asgc-gamejam-2026
```

## Git LFS Authentication

This repository stores large assets using Git LFS hosted on our Gitea server.

If you signed into Gitea using **Sign in with GitHub**, Git LFS may require a
local Gitea password for authentication.

### Set a Gitea Password

1. Log into Gitea using **Sign in with GitHub**.
2. Open **Settings → Account**.
3. Set a local password.
4. When Git prompts for Git LFS credentials, use:
   - **Username:** Your Gitea username (usually the same as your GitHub username)
   - **Password:** Your Gitea password

> **Note**
>
> We have not been able to consistently reproduce this issue, but several users
> have reported that Git LFS authentication fails until a local Gitea password
> has been configured.

## Download Git LFS Assets

Git LFS should automatically download project assets after cloning.

If not, fetch them manually:

```bash
git lfs pull
```

## Enable Git LFS Lock Verification

Enable lock verification for this repository:

```bash
git config lfs.locksverify true
```

Verify it is enabled:

```bash
git config --get lfs.locksverify
```

Expected output:

```text
true
```

## Verify Git LFS

Verify that Git LFS is configured correctly:

```bash
git lfs env
```

Verify that the lock server is reachable:

```bash
git lfs locks
```

If either command reports authentication errors, verify your Gitea credentials
and password before continuing.

## Open the Project

Open `ProjectAtlantis.uproject`.

If Unreal prompts you to build the project files, select **Yes**.

The first launch may take several minutes while Unreal generates project files,
compiles shaders, and prepares the project.

## Verify Your Setup

Before making changes, verify that:

- The project opens without errors.
- The project compiles successfully.
- You can launch the editor.
- You can load the default map.
- Git LFS assets downloaded successfully.
- `git lfs locks` succeeds without authentication errors.

If anything fails, ask for help before continuing.

## Next Steps

Once your environment is working:

- Read `CONTRIBUTING.md`.
- Pull the latest changes from `main`.
- Create a feature branch.
- Lock shared binary assets before editing them.
- Start building.
    -   MSVC
    -   Windows SDK
    -   C++ profiling tools (recommended)

Verify that Git LFS is installed

``` bash
git lfs version
```

If needed, initialize Git LFS

``` bash
git lfs install
```

## Clone the Repository

Clone the repository

``` bash
git clone https://github.com/ASGC-Game-Jam/asgc-gamejam-2026.git
cd asgc-gamejam-2026
```



Git LFS should automatically download the project assets, but if needed, pull them manually

``` bash
git lfs pull
```

Enable Git LFS lock verification for this repository

``` bash
git config lfs.locksverify true
```

Verify it is enabled

``` bash
git config --get lfs.locksverify
```

Expected output

``` text
true
```

## Open the Project

Open `ProjectAtlantis.uproject`

If Unreal prompts you to build the project files, select **Yes**

The first launch may take several minutes while Unreal generates project
files, compiles shaders, and prepares the project

## Verify Your Setup

Before making changes, verify that

-   The project opens without errors
-   The project compiles successfully
-   You can launch the editor
-   You can load the default map
-   Git LFS assets downloaded successfully

You can verify the lock server is reachable with

``` bash
git lfs locks
```

If anything fails, ask for help before continuing

## Next Steps

Once your environment is working

-   Read `CONTRIBUTING.md`
-   Pull the latest changes from `main`
-   Create a feature branch
-   Lock shared binary assets before editing them
-   Start working
