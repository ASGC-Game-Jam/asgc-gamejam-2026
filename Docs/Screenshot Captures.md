# Screenshot captures

Press the configured capture key (**F9** by default) with the game window focused to capture the game viewport, including
menus and HUD. This works on the main menu, during gameplay, and while paused,
including UI-only input modes. Hold-to-repeat is ignored.

In Shipping builds, first enable capture with the **`-debug` launch option**.
In Steam, open the game's **Properties > General > Launch Options**, enter
`-debug`, and restart the game. Remove the option and restart to disable capture.
For a standalone executable, launch `ProjectAtlantis.exe -debug`.
Non-Shipping builds enable capture automatically.

Captures use Unreal's built-in screenshot directory, normally
`Saved/Screenshots/<Platform>/`, as a PNG (EXR for HDR) and UTF-8 text file with the
same unique, timestamped basename. Attach both files to the bug report. Packaged builds may use
`%LOCALAPPDATA%/ProjectAtlantis/Saved/Screenshots/<Platform>/` instead of the build
directory. The engine's configured screenshot directory override is respected;
metadata always saves alongside the image.

The text includes UTC time, level name (without the PIE prefix), pawn name,
world-space XYZ coordinates in centimeters, and screenshot path. With no pawn
(for example, on the main menu), Player is `None` and Position is
`Unavailable (no possessed player pawn)`. The screenshot and level still save.
Coordinates are sampled at request time. Unreal captures the image and UI at the
end of the frame through `FScreenshotRequest`, the same path used by `shot showui`.
Metadata is written before queuing the image; if engine capture fails, the text
file can remain without an image. Verify both files before attaching a report.

## Capture key

Set **Project Settings > Game > Screenshot Capture > Capture Key**. The shortcut
is read from settings on each input event, so changes apply immediately in PIE.
Keyboard keys and gamepad buttons are supported. Set the key to
**None** to disable the shortcut while keeping the Blueprint capture action.

The project default lives in `Config/DefaultGame.ini`. For example, to use F8:

```ini
[/Script/ScreenshotCapture.AtlantisScreenshotCaptureSettings]
CaptureKey=F8
```

Packaged builds can override this section in their user `Game.ini` under
`Saved/Config/<platform>/Game.ini`; restart the game after editing it. Rebinding
does not require recompilation. Shipping still requires `-debug` at launch.
The old key stops triggering capture when it is replaced. The inherited engine
F9 screenshot binding remains suppressed to avoid an extra raw screenshot.

## Build and launch selection

| Build | Capture module | Capture active |
| --- | --- | --- |
| Editor / non-Shipping game | Included by default | Yes |
| Shipping | Included by default | Only with `-debug` at launch |
| Any target built with `-WithoutScreenshotCapture` | Excluded | Never, even with `-debug` |

To exclude capture entirely, pass the **build-time UBT flag**:

```powershell
& "$env:UE_PATH\Engine\Build\BatchFiles\Build.bat" ProjectAtlantis Win64 Shipping "-Project=$env:PROJECT_PATH" -WithoutScreenshotCapture
```

For the `BuildCookRun` command in [RELEASES.md](./RELEASES.md), append
`-ubtargs="-WithoutScreenshotCapture"`. Omit it to restore the default build.
The Editor target also accepts this build flag. Use separate archive folders
for variants. The previous `-WithScreenshotCapture` opt-in flag is no longer used.

The build flag sets `WITH_SCREENSHOT_CAPTURE=0` and excludes the entire module
from compilation, reflection, linking, and loading. Otherwise it is `1`.
The runtime `-debug` option activates Shipping capture only when it was compiled
in; it cannot restore code removed by the build flag. This option enables this
capture tool; it does not select a Debug build or enable all Unreal debug tools.

Focus checks select the appropriate PIE instance rather than editor panels.
Unreal handles capture, image encoding, and saving with UI enabled. In UE 5.7,
embedded PIE can include the surrounding editor window, matching `shot showui`.
Use a standalone game window or packaged build for game-only captures.
UI Blueprints can get **Atlantis
Screenshot Capture Subsystem** and call **Capture Screenshot**. In Shipping
without `-debug`, that subsystem is unavailable. `DefaultInput.ini` removes
Unreal's inherited F9 `shot showui` binding to prevent competing captures.
