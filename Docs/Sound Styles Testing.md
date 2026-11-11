# Testing Sound Styles

A test routine for the Sound Styles plugin (UI sounds chosen by tags and one style table) in ProjectAtlantis. Work through the parts **in order**: each part leaves the project in the state the next one expects, and the parts that change settings or the style table come last.

Design: [`Plugins/SoundStyles/UI Audio.md`](../Plugins/SoundStyles/UI%20Audio.md) · Usage: [`Plugins/SoundStyles/README.md`](../Plugins/SoundStyles/README.md) · Test kit details: [Appendix](#appendix-the-test-kit)

**Time:** about 20 minutes. **You need:** a mouse and, for part 4, a gamepad or the keyboard arrow keys.

| Part | What | Changes anything? |
|---|---|---|
| 1 | Setup | No |
| 2 | Automated tests | No |
| 3 | Editor checks | No (undo any test edit) |
| 4 | Play: normal use | No |
| 5 | Play: pause | No |
| 6 | Temporary table edits | **Yes**: undo each one |
| 7 | Shutdown and missing table | **Yes**: put the setting back |

For anything that fails, see [Reporting](#reporting).

## 1. Setup

1. Close the editor and **build the project**. The plugin builds from source with the game.
2. Open the project.
3. **Edit > Plugins**: Sound Styles **1.1** is enabled.
4. **Project Settings > Game > Sound Styles**: Style Table = `DA_AtlantisUISoundStyles`.
5. Open the **Output Log** and filter it by `LogSoundStyles`. Keep it visible during play.

## 2. Automated tests

- [ ] **Tools > Session Frontend > Automation**, filter `SoundStyles`, run all: **14 pass**.

## 3. Editor checks

No play needed. Undo any edit you make here (Ctrl+Z), or close without saving.

**Widgets**
- [ ] The UMG Palette has a **Sound Styles** category: Button, Slider, CheckBox, Progress Bar.
- [ ] Each has a **Sound Style** property in Details.
- [ ] A Sound Style Button has **no** Hovered/Pressed/Clicked Sound slots under Style.

**Style table**
- [ ] **Right-click in the Content Browser > Audio > Sound Styles > Sound Style Table** creates a table that opens straight into its editor, with a *(default)* row. Delete the new table afterwards.
- [ ] Open `DA_AtlantisUISoundStyles`. `Menu.Back`, `Pause` and `Journal` sit directly under *(default)*, and `Test.Child` sits under `Test`. Each row shows "N set".
- [ ] Select `Test.Child`: **Press** is highlighted, "this class (overrides Test)". The other events are plain, "inherited from Test".
- [ ] Select `Menu.Back`: **Press** is highlighted, "this class (overrides default)".
- [ ] Change any value, then **Ctrl+Z**: the change is undone.

## 4. Play: normal use

Open `L_SoundStylesTest` and press **Play**. In the console (`~`), run `SoundStyles.Debug 1`.

**Opening**
- [ ] The test widget appears with the mouse cursor.
- [ ] The first button gets focus when the screen opens, and **no** Hover plays for it.

**Button (mouse)**
- [ ] Mouse over a button → `MSS_UITest_Ping`. Log: `... / UISound.Event.Hover -> row UISound.Class.Test`.
- [ ] Click → ping.
- [ ] Press and hold → `MSS_UITest_Loop` plays. Release → it fades out, with no click.
- [ ] The button with an **empty** Sound Style also answers from row `UISound.Class.Test` (it picks the class up from the screen's scope).

**Button (gamepad or arrow keys)**
- [ ] Moving focus onto a button plays Hover.
- [ ] Clicking a button with the mouse plays Hover **once**, not twice.

**Slider**
- [ ] Mouse over the slider → **no** Hover (only Buttons play Hover).
- [ ] Grab the handle → Press, then the loop starts **at the handle's pitch** (no swoop up from low).
- [ ] Drag → the loop follows the handle, with a Step click at each 10%.
- [ ] Let go → the loop fades.
- [ ] Focus the slider and use the arrow keys → Step clicks, no loop.

**CheckBox**
- [ ] Click it on, then off → two different sounds (`MSS_UITest_Check` reads `Checked`). Log: row `UISound.Class.Test.Child`.

**Progress bar**
- [ ] Click **Fill** → the bar fills in about 4 seconds. The loop rises with it, with a Step click at each 10% (10 in a full run).
- [ ] The loop fades about 0.2 s after the bar is full.
- [ ] Click **Fill** again → it restarts from empty straight away.

**Parameters**
- [ ] The log shows `parameters from game: Value=...` under the slider and progress bar sounds.

Stay in Play for part 5.

## 5. Play: pause

- [ ] Toggle **Pause** on → the world is ducked (`CBM_UIState_Pause`).
- [ ] With Pause still on, click **Fill** → the bar still moves, and the loop still stops after it's full.
- [ ] Toggle Pause off → the world is back to normal.
- [ ] Toggle Pause **on** again and **stop PIE** with it on. Press **Play** again: the world is **not** ducked at the start.

Stop PIE.

## 6. Temporary table edits

Each check edits `DA_AtlantisUISoundStyles`. **Undo each edit before the next check**, and don't save the table.

**A misspelled parameter**
1. In the table, add a Parameter named `Valeu` to `Test` → Press.
2. Play, run `SoundStyles.Debug 1`, and click a button.
- [ ] The press plays at the MetaSound's default pitch, and the log shows `Valeu`.
3. Stop PIE and undo the edit.

**A one-shot through a level load**
1. Add a Parameter (any name) to `Test` → Press. A Press with parameters plays through an audio component instead of Slate.
2. Play, click a button, and straight away run `open L_SoundStylesTest` in the console.
- [ ] The Press sound **finishes** instead of cutting off.
3. Stop PIE and undo the edit.

**The ambiguity warning**
1. Add a row `Test.Other` with a Press sound. Give one test button two classes: `Test.Child` and `Test.Other`.
2. Play and run `SoundStyles.Debug 1`.
- [ ] The first press logs **one** "Ambiguous UI sound" warning. Pressing again logs no more.
3. Run `SoundStyles.Debug 0`.
- [ ] Pressing again logs **no** warning.
4. Stop PIE. Undo the row and the button's classes.

## 7. Shutdown and a missing table

**A loop at shutdown**
- [ ] Play, press and **hold** a button, then stop PIE while still holding → no loop left playing, no errors in the log.
- [ ] Play, hold a button, and run `open L_SoundStylesTest` → no loop left playing.

**No style table**
1. In **Project Settings > Game > Sound Styles**, clear **Style Table**.
2. Play and use the widgets.
- [ ] No crash, nothing plays, and **one** error in the log about the missing table.
3. Stop PIE and **set Style Table back** to `DA_AtlantisUISoundStyles`.

## Reporting

For anything that fails, note:
- which part and check
- which widget, and what you did
- what you heard
- the `LogSoundStyles` lines from the Output Log

## Appendix: the test kit

Everything is under `/Game/ProjectAtlantis/Audio/UI/Test/`:

| Asset | What it is |
|---|---|
| `L_SoundStylesTest` | Test map. Shows the test widget with the mouse cursor on begin play |
| `WBP_SoundStylesTest` | One of each Sound Style widget (Button, Slider, CheckBox, Progress Bar), a Pause toggle and a Fill button |
| `MSS_UITest_Ping` | Short one-shot, used for Hover and Press |
| `MSS_UITest_Loop` | Looping sound with a `Value` input (pitch follows it), used for Hold and Fill |
| `MSS_UITest_Check` | One-shot with a `Checked` input: on and off sound different |

The test rows (`UISound.Class.Test`, `UISound.Class.Test.Child`) live in `DA_AtlantisUISoundStyles`.

### Showing the widget with the mouse (Level Blueprint)

```
Event BeginPlay
 → Create Widget (Class: WBP_SoundStylesTest, Owning Player: Get Player Controller 0)
 → Add to Viewport
 → Set Input Mode UI Only (Player Controller: Get Player Controller 0, In Widget to Focus: the widget)
 → Set Show Mouse Cursor = true (target: Get Player Controller 0)
 → Set Keyboard Focus on the first button
```

- **Focus a button, not just the widget.** The gamepad and arrow keys can only move between buttons once one has focus. Focusing it straight after the screen opens also checks the settle window: it must not play Hover.
- **Is Focusable** must be ticked in `WBP_SoundStylesTest`'s Class Defaults if the widget itself is passed as Widget to Focus.
- **GameMode Override:** the map uses a plain `GameModeBase` (World Settings > GameMode Override). The project's own player controller sets game-only input on its own, which hides the cursor and swallows clicks.

### Moving the progress bar (`WBP_SoundStylesTest`)

The bar only makes sound through **Set Percent With Sound**; plain Set Percent stays silent. The widget drives it from **Event Tick**, not a timer, because widgets keep ticking while the game is paused but world timers stop, and part 5 needs the bar to move while paused.

Variables: `bFilling` (Boolean), `FillPercent` (Float, default 0).

```
On Clicked (FillButton)
 → Branch (FillPercent >= 1)
     True:  Set FillPercent = 0 ─┐
     False: ─────────────────────┴→ Set bFilling = true

Event Tick (InDeltaTime)
 → if bFilling:
      NewPercent = Min(FillPercent + (InDeltaTime * 0.25), 1.0)   // about 4 s from empty to full
      → ProgressBar → Set Percent With Sound (NewPercent)
      → Set FillPercent = NewPercent
      → if NewPercent >= 1: Set bFilling = false
```

Multiply `InDeltaTime` by the speed **before** adding it. `(FillPercent + InDeltaTime) * 0.25` shrinks the value every frame, and the bar stalls near zero.
