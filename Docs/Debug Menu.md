# Development debug menu

Available automatically with the Atlantis Player Controller in PIE, standalone,
and Debug / DebugGame / Development / Test builds, including packaged Steam QA builds.
In Shipping, launch with `-debug` to enable the menu, shortcuts, and restart.
For Steam, open **Properties > General > Launch Options**, enter `-debug`, and
restart the game. For a standalone Windows build, run `ProjectAtlantis.exe -debug`.
Remove the option and relaunch to disable the menu and restart shortcuts.

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Open menu | F7 | Hold left stick click + press Hamburger / Menu |
| Restart game | F10 | Right stick click while menu is open |
| Close menu | Escape / F7 | B / Circle / Hamburger / Menu |
| Navigate / activate | Arrow keys / Enter | D-pad / A / Cross |

Like Screenshot Capture, bindings use `UDeveloperSettings` and Game.ini:
**Project Settings > Game > Debug Menu** exposes keyboard and gamepad toggle /
restart keys and a gamepad toggle modifier. Set a key to None to disable that
shortcut, then restart PIE or the game. The default modifier avoids taking over
the Menu button's existing Input Action. Right-stick restart is available only
inside the menu. Keep keyboard bindings distinct from Screenshot Capture.
The menu displays the configured shortcuts.

Shortcuts use Enhanced Input actions and two mapping contexts built from these
settings. The gameplay context handles F7, F10, and the stick + Menu chord using
Unreal's chord trigger. While the debug menu is open, the controller withdraws
its gameplay mappings and applies the menu context for toggle, Back, and restart.
Closing restores gameplay mappings and ignores keys still held during the
transition. Navigation and button activation use the widget's normal UI handling.
The debug menu does not open while another menu is showing the mouse cursor or
gameplay controls are disabled; close that menu first. There is no custom Slate
input processor or pressed-key tracking.

Click **Restart Game** to restart without a shortcut. In builds with the
Unreal console, you can also enter `RestartGame`. In standalone play and
standalone PIE, restart uses Unreal's level travel,
closing the menu and reloading the current level. A fresh player and PlayerState
restore oxygen, ballast, position, and other level-owned state through normal
game initialization. GameInstance and saved data are not cleared. Network
sessions are not restarted by this command.

## Adding actions

Implement a no-argument `Exec` command on `UAtlantisCheatManager` (or an Unreal
`UCheatManagerExtension`) and append a label / console command to `DebugActions`.
For Blueprint features, derive an Atlantis Cheat Manager, configure the controller's
`CheatClass`, and edit the array or override `GetDebugActions`.
The menu builds its buttons from this list whenever opened. Check
`UAtlantisDebugMenuSettings::IsAvailable()` at any command entry point meant
to work with `-debug`. Keep development-only commands behind their existing
build guards.

## Packaged Shipping smoke test

Use a Shipping package from the [Release Process](./RELEASES.md). Enter a
playable level for each run.

1. Launch with no options. F7 and the left-stick click + Menu chord should
   leave the debug menu closed, and F10 should leave the level running.
   Check that Menu alone still opens the journal.
2. Relaunch with `-debug`. Open and close with F7, then open with the controller
   chord and close with Menu. Check that movement and journal input resume.
   Open another UI such as the journal and check that F7 leaves it in control.
3. Reopen the menu for each restart method and try F10, **Restart Game**,
   and right-stick click. Each should reload the level and restore the player
   to the starting state. Right-stick click outside the menu should keep its
   normal gameplay behavior.
4. Repeat both launch configurations in Steam using **Launch Options**.
