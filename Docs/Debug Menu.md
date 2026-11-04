# Development debug menu

Tracks [Build Debug Menu #293](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/issues/293).

Available automatically with the Atlantis Player Controller in PIE, standalone,
and Debug / DebugGame / Development / Test builds, including packaged Steam QA builds.
Shipping production builds have no menu contents, shortcuts, or functional debug
commands; the server reset body is also compiled out.
No Blueprint or map edits are needed.

This branch is stacked on [screenshot capture PR #303](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/pull/303).
F9 remains owned by its screenshot module, including when this menu has focus.
The menu does not replace its input processor, capture implementation, or module
loading / exclusion rules.

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Open menu | F7 | Hold left stick click + press View / Back |
| Reset immediately | F10 | Right stick click while menu is open |
| Close menu | Escape / F7 | B / Circle / View / Back |
| Navigate / activate | Arrow keys / Enter | D-pad / A / Cross |

Like Screenshot Capture, bindings use `UDeveloperSettings` and Game.ini:
**Project Settings > Game > Debug Menu** exposes keyboard and gamepad toggle /
reset keys and a gamepad toggle modifier. Set a key to None to disable that
shortcut, then restart PIE or the game. The default modifier avoids taking over
the View button's existing Input Action. Right-stick reset is available only
inside the menu. Keep keyboard bindings distinct from Screenshot Capture.
The menu displays the configured shortcuts.

The panel is 720 units wide with a 32-point heading, 24-point action labels,
larger buttons, and a high-contrast background. It scales down to fit small
windows and scrolls as more actions are added.

Click **Reset Player to Start** for the same action, or enter `ResetPlayer` in the
Unreal console (use `EnableCheats` first in network sessions if needed).

Reset teleports the existing Atlantis character to its BeginPlay position and
rotation, clears pending movement input and velocity, restores camera facing, and
selects the default movement mode for the destination volume. Unreal handles
collision adjustment; if the destination is blocked and no valid position can be
found, the character stays where it is. After a successful teleport, ballast
returns to its PlayerState class default (Descend by default), updating its oxygen
reservation through the normal state setter. Inventory and current oxygen are
preserved. Reset is not a respawn or a level reload.

The owning controller sends reset to the server in network PIE. The menu belongs
to each local player. Menu input suppression is balanced with Unreal's move/look
ignore counters; it does not remove Enhanced Input contexts or release another
system's restrictions. While open it uses UI-only input and flushes held gameplay
keys. A Slate input preprocessor scopes debug controls to the owning local player
and game viewport, ahead of CommonUI / Enhanced Input. It consumes both the
press and release of B and other debug shortcuts, including the release after
the menu has closed, so one button cannot also trigger a gameplay Input Action.
Unmodified View and right-stick presses pass through normally outside the menu.
Open it from gameplay: it refuses to take ownership while
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
8. With the menu focused, press F9. Verify #303 still saves a screenshot and
   matching location metadata. Change the Debug Menu key, restart PIE, and verify
   the configured key and displayed hint agree.
9. Verify View and right-stick gameplay actions work with the menu closed.
   Hold left-stick click and press View to open it; verify View does not also
   trigger its Input Action. Close with B while swimming and verify no swim action
   occurs on either press or release. Hold B and verify it closes only once.
   Repeat with different focused buttons and after clicking empty menu space.

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

#303's Shipping `-debug` option enables screenshot capture only. It does not
enable this menu or cheats. The packaging helper forwards
`-WithoutScreenshotCapture` to UBT to omit #303's optional module when requested.

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

## Stack and readability update (2026-10-15)

Stacked locally on #303 at `254e6a3`; the screenshot module remains unchanged.
Reset tests now use isolated fixtures, Describe / It cases, and BeforeEach /
AfterEach, matching #303 and the repository's current test convention. Added
isolated config-binding specs for remapping and disabling shortcuts.

Only source / diff checks and PowerShell syntax checks were performed for this
update. No rebuild or automation execution was requested; the prior build and
test results above do not validate the new settings, spec conversion, or layout.

The gamepad routing fix adds `Atlantis.Debug.InputRouting` spec coverage for
modifier chords, normal gameplay pass-through, B press / release across closing,
repeat suppression, menu-only reset, and leaving F9 to screenshot capture.
These new specs have not been built or executed; hardware PIE verification
of the checklist above is still required.
