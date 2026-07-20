# Development Setup

This guide walks through setting up your development environment for the ASGC Game Jam 2026 project.

## Requirements

Install the following before cloning the repository:
- Git
- Git LFS
- Unreal Engine 5.7.4
- Visual Studio 2022 or some other IDE for Game Development with C++
  - MSVC
  - Windows SDK
  - C++ profiling tools (recommended)

## Clone the Repository

Clone the repository normally.

```bash
git clone https://github.com/ASGC-Game-Jam/asgc-gamejam-2026.git
cd asgc-gamejam-2026
```

Git LFS will automatically download project assets.

Verify LFS is installed:

```bash
git lfs version
```

If needed:

```bash
git lfs install
```

## Open the Project

Open `ProjectAtlantis.uproject`.

If Unreal asks to build project files, select **Yes**.

The first launch may take several minutes while Unreal generates project files and shaders.

## Verify Your Setup

Before making changes, confirm that:
- The project opens without errors
- The project compiles successfully
- You can launch the editor
- You can load the default map

If any of these steps fail, ask for help before continuing.

## Next Steps

Once your environment is working:
- Read `CONTRIBUTING.md`
- Create a feature branch
- Start working
