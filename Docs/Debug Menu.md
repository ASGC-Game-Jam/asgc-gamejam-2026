# Development debug menu

Tracks [Build Debug Menu #293](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/issues/293).

Available automatically with the Atlantis Player Controller in PIE, standalone,
and Debug / DebugGame / Development / Test builds, including packaged Steam QA builds.
Shipping production builds have no menu contents, shortcuts, or functional debug
commands; the server reset body is also compiled out.
No Blueprint or map edits are needed.

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Open / close menu | F7 | View / Back |
| Reset immediately | F10 | Right stick click |
| Close menu | Escape | B / Circle |
| Navigate / activate | Arrow keys / Enter | D-pad / A / Cross |

Click **Reset Player to Start** for the same action, or enter `ResetPlayer` in the
Unreal console (use `EnableCheats` first in network sessions if needed).

Reset teleports the existing Atlantis character to its BeginPlay position and
rotation, clears pending movement input and velocity, restores camera facing, and
selects the default movement mode for the destination volume. Unreal handles
collision adjustment; if the destination is blocked and no valid position can be
found, the character stays where it is. Inventory, oxygen, ballast and other
gameplay state are preserved. Reset is not a respawn or a level reload.

The owning controller sends reset to the server in network PIE. The menu belongs
to each local player. Menu input suppression is balanced with Unreal's move/look
ignore counters; it does not remove Enhanced Input contexts or release another
system's restrictions. Open it from gameplay: it refuses to take ownership while
another menu is displaying the mouse cursor. Close it before opening another UI.

## Adding actions

Implement a no-argument `Exec` command on `UAtlantisCheatManager` (or an Unreal
`UCheatManagerExtension`) and append a label / console command to `DebugActions`.
For Blueprint features, derive an Atlantis Cheat Manager, configure the controller's
`CheatClass`, and edit the array or override `GetDebugActions`.
The menu builds its buttons from this list whenever opened. Keep all new cheat
implementations behind `!UE_BUILD_SHIPPING`, including any server RPC body.

## QA checks

1. In PIE, move away from spawn, look elsewhere, then press F10 while moving or
   falling. Verify return to spawn, original facing, and zero residual velocity.
2. Open with F7, click reset, navigate buttons with arrows / Enter, then close.
   Verify normal movement/look resumes. Repeat with a gamepad.
3. Reset between land and water. Verify traversal mode and input match the destination.
4. Disable gameplay controls before opening. Verify reset works and closing does
   not enable those controls. Reopen repeatedly to check balanced suppression.
5. Possess a fresh character and reset; verify it uses that character's start.
   Try with no pawn and with a blocked spawn; verify no crash.
6. In two-player network PIE, reset each client and verify the server and other
   client see its corrected position. Test split-screen focus per local player.
7. In a Shipping package, verify menu keys and reset commands have no effect.

Automated reset regression: `Atlantis.Debug.ResetPlayer`.

## Production and Steam QA packaging

Use `Scripts/PackageGame.ps1 -Channel QA -EnginePath <Unreal installation>` to
package Development for the Steam `qa` branch. `-Configuration Test` is also
supported. Use `-Channel Production` for Shipping and the Steam `default` branch.
The Epic Launcher engine distribution supports Development QA; Test requires
an engine distribution with that configuration available.
The script rejects QA Shipping and production Development / Test combinations,
and archives them into separate `Saved/Packages/QA` and
`Saved/Packages/Production` folders.

Steam branch selection does not change a compiled executable's capabilities.
Upload the QA package only to `qa`; build a separate Shipping package for
production. Never promote the QA artifact to `default`. See
[Release Process](./RELEASES.md) for commands and upload steps.

## Implementation validation (2026-10-11)

- UE 5.7.4 Win64 Development Editor, Development game, and Shipping game: built successfully.
- `Atlantis.Debug.ResetPlayer`: passed in Unreal headless automation. Covers
  initial position / facing, velocity and pending-input clearing, repeated reset,
  no-pawn safety, upgrading the existing controller Cheat Manager default, native
  menu construction and keyboard activation of its reset button.
- Packaging script: PowerShell parse check passed; production Development and
  QA Shipping requests were rejected before invoking UAT.
- Test configuration could not be compiled with this installed engine distribution.
- Full cooking / packaging, Steam upload, hardware gamepad input, visual PIE,
  water transitions, blocked spawn, and network / split-screen checks remain manual.
