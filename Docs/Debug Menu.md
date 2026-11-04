# Development debug menu

Available automatically with the Atlantis Player Controller in PIE, standalone,
and Debug / DebugGame / Development / Test builds, including packaged Steam QA builds.
Shipping production builds have no menu contents, shortcuts, or functional debug
commands.

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Open menu | F7 | Hold left stick click + press Hamburger / Menu |
| Reset immediately | F10 | Right stick click while menu is open |
| Close menu | Escape / F7 | B / Circle / Hamburger / Menu |
| Navigate / activate | Arrow keys / Enter | D-pad / A / Cross |

Like Screenshot Capture, bindings use `UDeveloperSettings` and Game.ini:
**Project Settings > Game > Debug Menu** exposes keyboard and gamepad toggle /
reset keys and a gamepad toggle modifier. Set a key to None to disable that
shortcut, then restart PIE or the game. The default modifier avoids taking over
the Menu button's existing Input Action. Right-stick reset is available only
inside the menu. Keep keyboard bindings distinct from Screenshot Capture.
The menu displays the configured shortcuts.

Shortcuts use Enhanced Input actions and two mapping contexts built from these
settings. The gameplay context handles F7, F10, and the stick + Menu chord using
Unreal's chord trigger. While the debug menu is open, the controller withdraws
its gameplay mappings and applies the menu context for toggle, Back, and reset.
Closing restores gameplay mappings and ignores keys still held during the
transition. Navigation and button activation use the widget's normal UI handling.
The debug menu does not open while another menu is showing the mouse cursor or
gameplay controls are disabled; close that menu first. There is no custom Slate
input processor or pressed-key tracking.

Click **Reset Player to Start** for the same action, or enter `ResetPlayer` in the
Unreal console (use `EnableCheats` first in network sessions if needed).

Reset teleports the existing Atlantis character to its BeginPlay position and
rotation, clears pending movement input and velocity, restores camera facing, and
selects the default movement mode for the destination volume. After a successful
teleport, ballast returns to its PlayerState class default, updating its oxygen
reservation through the normal state setter. Inventory and current oxygen are
preserved. Reset is not a respawn or a level reload.

## Adding actions

Implement a no-argument `Exec` command on `UAtlantisCheatManager` (or an Unreal
`UCheatManagerExtension`) and append a label / console command to `DebugActions`.
For Blueprint features, derive an Atlantis Cheat Manager, configure the controller's
`CheatClass`, and edit the array or override `GetDebugActions`.
The menu builds its buttons from this list whenever opened. Keep all new cheat
implementations behind `!UE_BUILD_SHIPPING`, including any server RPC body.

## Production and Steam QA packaging

Package Development for the Steam `qa` branch so the debug menu is available.
Use Shipping for production, where the menu and cheats are disabled. Follow the
existing [Release Process](./RELEASES.md) to build and upload each package.

Steam branch selection does not change a compiled executable's capabilities.
Upload the QA package only to `qa`; build a separate Shipping package for
production. Never promote the QA artifact to `default`.
