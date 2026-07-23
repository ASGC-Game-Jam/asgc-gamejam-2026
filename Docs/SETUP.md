# Development Setup

This guide walks through setting up your development environment for the
ASGC Game Jam 2026 project.

## Requirements

Install the following before cloning the repository

-   Git
-   [Git LFS](https://git-lfs.com/)
-   Unreal Engine 5.7.4
-   Visual Studio 2022 or another IDE configured for Game Development
    with C++
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

Open [`ProjectAtlantis.uproject`](../ProjectAtlantis.uproject)

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

-   Read [CONTRIBUTING.md](../CONTRIBUTING.md)
-   Pull the latest changes from `main`
-   Create a feature branch
-   Lock shared binary assets before editing them
-   Start working
