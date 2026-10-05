# Automated Tests

The project uses Unreal Engine's Automation System for C++ tests. Tests guarded by
`WITH_DEV_AUTOMATION_TESTS` are available in Development Editor builds.

## Run tests in the Unreal Editor

1. Build and run the project.
2. Open **Tools → Session Frontend → Automation**.
3. Search for tests starting with `Atlantis`, select the checkboxes of the tests you want to run, and click the play button to **Start Tests**.
4. Select the test row to review the results. Expected result: **Success**.

## Run tests from the command line

Unreal Editor can run the same tests unattended. Build the project's Development Editor target first, then use the
engine's editor executable, the project file, and the test name as a `RunTests` filter. Adjust the engine and project
paths in the examples below.

**Note**: I only ran the Windows command to validate it. The other ones are untested.

### Windows (PowerShell)

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\path\to\asgc-gamejam-2026\ProjectAtlantis.uproject" `
  -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests Atlantis.PlayerState.BallastAllocation" `
  -TestExit="Automation Test Queue Empty"
```

### macOS (Terminal)

Run the executable inside the editor's app bundle:

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "/path/to/asgc-gamejam-2026/ProjectAtlantis.uproject" \
  -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests Atlantis.PlayerState.BallastAllocation" \
  -TestExit="Automation Test Queue Empty"
```

### Linux (Terminal)

Use the editor executable from your engine installation or source build:

```bash
"/path/to/UnrealEngine/Engine/Binaries/Linux/UnrealEditor" \
  "/path/to/asgc-gamejam-2026/ProjectAtlantis.uproject" \
  -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests Atlantis.PlayerState.BallastAllocation" \
  -TestExit="Automation Test Queue Empty"
```

`-TestExit="Automation Test Queue Empty"` exits the editor after the tests finish.
Replace the test name with `Atlantis` to run all tests in the project's group.

The test log is written under the project's `Saved/Logs` directory. A
`WITH_DEV_AUTOMATION_TESTS` build configuration is required for development
tests to be included.
