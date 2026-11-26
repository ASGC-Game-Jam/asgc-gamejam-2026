# UI Audio: Sound Styles

> **Status: plugin built (v1.0, Nov 2026); game integration not started.** The system is **Sound Styles**, a reusable MIT-licensed plugin owned by Clara Smith; see [Plugin](#plugin). Everything in the plugin column of [What lives where](#what-lives-where) exists and is tested. The game-side steps in the [Implementation plan](#implementation-plan) are still to do.

UI sounds work like CSS. Widgets are tagged with **sound classes** (gameplay tags), and one table that audio owns, the **stylesheet**, decides what each class sounds like. UI developers never pick sound assets, and audio can re-skin every button in the game without touching a widget.

```
WBP_PauseMenu          scope: Menu
  ReturnToGameButton   classes: Menu.Primary
  ReturnToMenuButton   classes: Menu.Back
  QuitButton           (none: uses the screen's Menu scope)
```

## Why

- **Little or nothing per button.** A screen sets a scope once, and its buttons pick it up. A button only needs a class when it should sound different.
- **Consistent.** Every "back" button sounds the same because they share a class, not because someone copied the same asset twelve times.
- **Audio owns the sounds.** Changing what a primary menu button sounds like is one edit in one table.
- **Gamepad included.** Moving focus with the D-pad plays the hover sound, which plain UMG buttons don't do.
- **More than one-shots.** A screen can also change the mix while it's open, for example ducking the world under the pause menu. Held buttons, slider drags and filling progress bars play loops.
- **Game data drives the sound.** Sliders, checkboxes and progress bars send their state (`Value`, `Checked`) to MetaSound inputs, and game code can send its own values.

## Concepts

### Classes and events

| Tag root | Meaning | Examples |
|---|---|---|
| `UISound.Class` | **Class**: what kind of element this is | `UISound.Class.Menu.Primary`, `UISound.Class.Journal.Tab` |
| `UISound.Event` | **Event**: what just happened to it | `UISound.Event.Hover`, `UISound.Event.Press`, `UISound.Event.Open` |

The plugin defines `Hover` (mouse over or gamepad focus), `Press`, `Release`, `Open`, `Close`, and three for continuous widgets: `Hold` (a loop while a button is pressed or a slider dragged), `Step` (a slider or progress bar crossing a detent) and `Fill` (a loop while a progress bar moves). The game adds its own, such as `Select` (tabs), `Notify` and `Type`. Nothing in code lists them: the table editor shows every tag directly under `UISound.Event`. Tag names below drop the `UISound.Class.` / `UISound.Event.` prefix for readability.

### Classes inherit from their parent tag

Like CSS properties, each event inherits from the parent class unless the class sets it.

```
(root)                 Hover: tick     Press: click
└─ Menu                                Press: thunk
   ├─ Primary                          Press: big brass thunk
   │  └─ NewGame                       Press: fanfare
   └─ Back                             Press: soft close
```

A button with class `Menu.Back`:
- **Press:** `Menu.Back` sets it, so it plays the soft close.
- **Hover:** neither `Menu.Back` nor `Menu` sets it, so it falls through to the root and plays the tick.

A new class only lists what's different. The root (`UISound.Class` itself) is the game-wide default.

An event can be set to **silent**, which stops it inheriting a sound. For example, journal tabs set Press to silent and play `Select` only when the tab actually changes.

### A widget can have several classes

Like `class="menu primary"`, a widget can have more than one class. For each event, the **most specific** class that sets it wins: the deepest tag in the hierarchy. To make one button different, add a child class for it, such as `Menu.Primary.NewGame`, and it inherits everything else from its parent.

If two of a widget's classes set the same event at the same depth, that's ambiguous. The first one listed wins, and with `SoundStyles.Debug 1` a warning is logged once for each pair of rows.

### Scopes: classes from the screen

A screen (or any widget that contains others) can declare a **scope**: classes its child widgets inherit, like a CSS descendant selector. A button with no classes of its own uses the nearest scope above it.

For each event, the order is:
1. The widget's own classes
2. The nearest enclosing scope's classes, then the next scope up, and so on
3. The root default

So `QuitButton` in the pause menu above uses `Menu`, and `ReturnToMenuButton`'s own `Menu.Back` beats the scope. Wrapper widgets like `WBP_MenuButton` need no extra plumbing: the inner button finds the scope itself.

### One-offs and custom sounds

Every sound is chosen in the table, never on a widget:

- **One-off:** add a child class for the one button, setting only what's different (`Menu.Primary.NewGame` above).
- **A widget with its own sounds:** give it a class and have it play **its own event tags**. A custom event works like a custom sound variable, but the sound is still picked in the table. Example: `Journal.Clue` with `Expand: paper unfold` and `Collapse: paper fold`, played by `WBP_JournalClueRow`.
- **Classes chosen from data:** classes are ordinary tags, so they can come from game data at runtime. Example: the dialogue box uses `Dialogue.Speaker.Engineer` from the current line's speaker.

### Screen audio states

A class can also name a **control bus mix** that is active while a screen of that class is open. Screens push the state when they open and pop it when they close. States stack, so journal, then pause, then back works.

```
Pause      while open: CBM_UIState_Pause    (world ducked)
Journal    while open: CBM_UIState_Journal  (world muffled)
```

State mixes are **separate from `CBM_User`**, the player's volume settings. When two active mixes set the same volume bus, Audio Modulation multiplies them, so a state mix scales whatever the player chose and the settings never change. Muffling needs a low-pass bus on the world sound classes, which they don't have yet.

## Using it

### UI developers: a screen

1. Add the **Sound Style Scope Interface** to the screen widget and return its class from **Get UI Sound Classes**, for example `Menu`.
2. In the screen's open and close handlers, call **Play Widget UI Sound** with `Open` / `Close`, and **Push** / **Pop UI Audio State** if the screen should change the mix.

### UI developers: a button (or slider, checkbox, progress bar)

1. The widgets are in the Palette under **Sound Styles**: **Sound Style Button**, **Slider**, **CheckBox** and **Progress Bar**. To convert an existing Button, select **Sound Style Button** in the Palette (click it once, don't drag it). Then right-click the Button in the Hierarchy and choose **Replace With… > Replace With SoundStyleButton**. The slot, style and bindings carry over. The same works for sliders, checkboxes and progress bars.
   > The **Replace With…** list itself only shows multi-child panels. The entry for a normal widget only appears at the top of the menu, and only while that widget is selected in the Palette.
2. Leave **Sound Style** empty to use the screen's scope. Set it only when the widget should sound different.
3. The Button Style's own **Hovered/Pressed/Clicked Sound** slots are hidden. If one was already set, it's cleared and a warning explains why. The table plays those sounds instead.
4. **Progress bars:** call **Set Percent With Sound** instead of Set Percent when the change should be heard. Plain Set Percent stays silent.

If no existing class fits, ask audio for one rather than adding sounds to the widget.

### Audio: adding or changing a class

1. Open `DA_AtlantisUISoundStyles` (a **Sound Style Table**; new ones come from **right-click > Audio > Sound Styles > Sound Style Table**). The table editor shows rows on the left and the selected row's events on the right.
2. **+ Add Row**, then pick the class in the row's tag picker. Its **+** creates a new tag under `UISound.Class` if needed.
3. Pick sounds only for the events that differ from the parent. The **From** column shows where every other event comes from. **Silence** makes an event play nothing; **Reset** makes it inherit again.
4. Make sound presets from `MSS_Stereo_OneShot_lib` (#65). They route to `SC_UI` and `Submix_UI` with high priority. `Hold` and `Fill` need looping sounds.

No code changes and no widget changes.

### Audio: sounds that react to game data

A MetaSound with matching inputs receives the widgets' data: `Value` (float, 0–1) from sliders and progress bars, and `Checked` (bool) from checkboxes. Game code can send its own with **Play Widget UI Sound With Parameters**, **Start UI Loop With Parameters** and **Set UI Loop Parameters**.

Each event in the table can also carry fixed **Parameters**: select the event, and edit them in the panel underneath. Use them for per-class differences with one shared MetaSound, for example `Brightness` 0.2 on `Menu` and 0.8 on `Menu.Back`. The table's values are sent first and the game's after, so where both use the same name, **the game wins**.

Names must match the MetaSound's inputs exactly. Unknown names are ignored without an error, which is why the debug output lists them.

### Finding the row to edit

Set `SoundStyles.Debug 1` in the console. Each UI sound then logs the widget, the event, which row answered, and any parameters sent:

```
QuitButton / UISound.Event.Press -> row UISound.Class.Menu
NewGame / UISound.Event.Press -> row UISound.Class.Menu.Primary.NewGame
VolumeSlider / UISound.Event.Hold (loop) -> row UISound.Class.Menu
    parameters from game: Value=0.80
```

## What is and isn't a UI sound

Rule of thumb: **if the sound should still play with the HUD turned off, it isn't a UI sound.**

| Sound | Path |
|---|---|
| Button hover, press, tab select | UI: Sound Style Button and classes |
| Screens opening and closing | UI: Play Widget UI Sound |
| World ducked under pause, muffled under journal | UI: screen audio states |
| Journal updated, clue found, objective complete | UI: `Notify`, played by the widget that **shows** it. Notifications can arrive while the journal is closed, so this needs a HUD notification widget, which doesn't exist yet |
| Panel slide-in timed to an animation | UI: UMG animations can't hold audio tracks, but an **Event track** can call Play UI Sound on the right frame |
| Dialogue typewriter | UI: `Type`, one sound per letter (below) |
| Meter tick as the oxygen bar animates | UI: from `WBP_OxygenIndicator`, e.g. a Sound Style Progress Bar's `Step` and `Fill` |
| Options menu sliders and toggles | UI: Sound Style Slider and CheckBox |
| Oxygen low or critical alarm, breathing strain | **Player feedback**, from `Audio.Fact.Oxygen.Level` / `Oxygen.Critical` |
| Oxygen refill at a membrane (#80), ballast valve | **Player feedback**, from facts or the gameplay event |

**Player feedback is not part of this system.** It listens to the same audio facts as the music (the fact bridge, #489, already sends the oxygen facts), so the alarm stays audible while the journal, a cutscene or the debug menu hides the HUD, and stays in step with the music. It gets its own design.

## Plugin

The system is built as a plugin, **Sound Styles**, so it has no dependency on game code and can be reused in other projects. The game uses it like any other plugin.

### Ownership and license

Sound Styles is **Clara Smith's own plugin, licensed MIT**, and developed in its own repository (`clarasmith-sound/SoundStyles`), not in this one. ASGC uses it under the MIT license, which is compatible with this project's AGPL-3.0.

- **Its files carry `Copyright (c) 2026 clarasmith-sound`** (Clara's GitHub name) with an MIT SPDX line, not the ASGC header, and the plugin folder includes its `LICENSE` file.
- **ASGC includes a release copy** in `Plugins/SoundStyles`, not a git submodule, so nobody needs extra clone steps. The plugin's README in that folder records its source repository and version.
- **Changes go upstream first.** Fix or extend the plugin in its own repository, then a PR here copies in the new version. Don't edit `Plugins/SoundStyles` in this repo directly, or the next update will overwrite the change.
- **Contributions are welcome** as PRs to the plugin's repository, under the same MIT license.
- **No ASGC code goes into the plugin.** It's written fresh, so it stays clean to license separately. Game code that uses it (the style table, classes, widget setup, player feedback) stays in this project under AGPL.

### What lives where

| Plugin (`Plugins/SoundStyles`) | Game (ProjectAtlantis) |
|---|---|
| `USoundStyleTable`, `FSoundStyleClass`, `FSoundStyleEvent` (the data types) | `DA_AtlantisUISoundStyles` (the table asset and its rows) |
| `USoundStyleSubsystem` (resolution, playback, loops, parameters, state stack, debug output) | Class tags: `UISound.Class.Menu`, `UISound.Class.Journal`, ... |
| Widgets: `USoundStyleButton`, `USoundStyleSlider`, `USoundStyleCheckBox`, `USoundStyleProgressBar` | Game-specific events: `Select`, `Notify`, `Type`, ... |
| `ISoundStyleScopeInterface` (screens) and `ISoundStyleWidgetInterface` (the plugin's widgets) | Sound presets (from `MSS_Stereo_OneShot_lib`), MetaSounds with `Value` / `Checked` inputs, state mixes (`CBM_UIState_*`) |
| `USoundStyleSettings`: **Project Settings > Game > Sound Styles**, where the project picks its table | Widgets: scopes, classes and Play UI Sound calls |
| Root tags `UISound.Class` and `UISound.Event`, plus the events the plugin plays itself: `Hover`, `Press`, `Release`, `Open`, `Close`, `Hold`, `Step`, `Fill` | Player feedback (not part of this system) |
| Editor: the table editor, the "new table" menu entry, the Button Details customization | |
| Automated tests for resolution and the state stack | |

The rule: **the plugin knows how sounds are chosen and played, the game decides which sounds and where.** Nothing in the plugin names a game class, asset or tag below the roots.

### Structure

```
Plugins/SoundStyles/
  LICENSE                       MIT
  README.md                     usage guide for anyone installing the plugin
  SoundStyles.uplugin           depends on the AudioModulation plugin (and GameplayTagsEditor, editor only)
  Source/SoundStyles/           runtime module
    Public/   SoundStyleTable.h, SoundStyleSubsystem.h, SoundStyleSettings.h, SoundStyleTags.h,
              SoundStyleButton.h, SoundStyleSlider.h, SoundStyleCheckBox.h, SoundStyleProgressBar.h,
              SoundStyleScopeInterface.h, SoundStyleWidgetInterface.h
    Private/  ... and Tests/
  Source/SoundStylesEditor/     editor module: the table editor, its asset definition and factory,
                                and the Sound Style Button Details customization
```

- **Runtime module dependencies:** public `Core`, `CoreUObject`, `Engine`, `GameplayTags`, `UMG`, `DeveloperSettings`, `AudioExtensions` (for `FAudioParameter` in Blueprint functions); private `Slate`, `SlateCore`, `AudioModulation`.
- **Game side:** enable the plugin in `ProjectAtlantis.uproject` and add `SoundStyles` to the game module's dependencies, only if game C++ calls it. Blueprint-only use needs just the plugin enabled.
- **Settings move into the plugin.** The table is no longer set in Atlantis Audio Settings, because the plugin can't see game settings.
- **Tests** are named `SoundStyles.*`, so they run separately from the game's `Atlantis.*` tests.
- **Keep it generic:** game-specific needs go in game code or data, not in the plugin.

## Design details

### Data

```cpp
USTRUCT(BlueprintType)
struct FSoundStyleEvent
{
    // Empty means silent: stops this event inheriting from the parent class.
    UPROPERTY(EditAnywhere)
    TObjectPtr<USoundBase> Sound;

    // Optional fixed MetaSound parameters, sent when the sound starts. Game values override them by name.
    UPROPERTY(EditAnywhere)
    TArray<FAudioParameter> Parameters;
};

USTRUCT(BlueprintType)
struct FSoundStyleClass
{
    UPROPERTY(EditAnywhere, meta = (Categories = "UISound.Event"))
    TMap<FGameplayTag, FSoundStyleEvent> Events;

    // Active while a screen of this class is open. Optional.
    UPROPERTY(EditAnywhere)
    TObjectPtr<USoundControlBusMix> WhileOpenMix;
};

UCLASS()
class USoundStyleTable : public UPrimaryDataAsset
{
    UPROPERTY(EditAnywhere, meta = (Categories = "UISound.Class"))
    TMap<FGameplayTag, FSoundStyleClass> Classes;
};
```

- **Missing event:** inherits. **Event present with no sound:** silent, and resolution stops. An event inherits as a whole (sound and parameters together); the table editor copies an inherited event onto the class before changing either.
- **Hard references** to the sounds, so all UI sounds load with the table. They're small, and a hover can't wait for an async load.
- **`Categories`** limits each tag picker to the right root.

### Resolving a sound

```
Resolve(widget classes, scope chain, Event):
    for each level in [widget's own classes, nearest scope, next scope up, ...]:
        best = none
        for each class at this level:
            walk from the class up its parents, stopping BEFORE the root
            the first row that sets Event is this class's answer
            keep the answer from the deepest row; warn on a tie
        if best found -> return it (may be silent)
    return the root's answer for Event (may be nothing)
```

The root is excluded from the per-class walks, so a widget's own classes can't hide its scope behind the global default. Scopes are found each time a sound plays, by walking up parent widgets (across nested user widgets) to each widget implementing the scope interface. That's cheap for menu-sized trees, and it means a widget moved to another screen picks up the new scope with no extra work. A class outside `UISound.Class` logs a warning and is ignored.

The lookup lives on the table (`USoundStyleTable::Resolve`), not the subsystem, so the table editor uses the same code to show where each event comes from.

### Playback

Sounds play through `FSlateApplication::PlaySound`, the same path UMG uses for its built-in button sounds. It plays with no world, so a press sound isn't cut off when the button starts a level load. It's flagged as a UI sound, so it plays while paused, and it uses the asset's priority. `UGameplayStatics::PlaySound2D` would tie the sound to the current world and cut it off on level travel.

Because it's a normal active sound, the sound's **Sound Concurrency** applies. Use it (Retrigger Time, Max Count) to limit fast repeats, such as sweeping across a list; the plugin has no interval setting of its own.

Two cases need an **audio component** (`UGameplayStatics::CreateSound2D`, flagged as a UI sound) instead, because Slate sounds can't be stopped or given parameters:
- **Loops** (`Hold`, `Fill`). The subsystem keeps one per widget, and stops it on release, when the widget is torn down, and on level travel. Stopping fades out over 50 ms, so it doesn't click.
- **Sounds with parameters**, from the game or from the table. Parameters are set **before** Play, so the first sample already has them.

All timing (the focus and load settle windows, the progress bar's quiet time) uses **real time**, because the journal and pause menu pause the game. For the same reason, the progress bar's quiet check runs on the engine's core ticker, not the world's timers, which stop while paused.

### Subsystem

`USoundStyleSubsystem` is a game instance subsystem, so it lives from the main menu through every level. It:

- loads the table set in **Project Settings > Game > Sound Styles** in `Initialize` and keeps it loaded. It logs an error if the table isn't set.
- plays sounds: **Play UI Sound** (classes + event) and **Play Widget UI Sound** (a widget + event, which uses the widget's classes and scopes), plus **Play Widget UI Sound With Parameters**.
- runs loops: **Start UI Loop** (with or without parameters), **Set UI Loop Parameters**, **Stop UI Loop**, **Stop All UI Loops**.
- keeps the **screen audio state stack**, with each mix reference-counted so two screens sharing a mix don't cancel each other.
- **clears every state on level travel.** A screen can open a level without closing first (pause menu > Return to Main Menu), and a state left active would duck the next level.
- **drops UI sounds while a level loads** and for 0.5 s after, so restored journal entries don't chime. It also stops every loop when a level starts loading.
- does nothing where Slate isn't running (automated tests, commandlets).

### Button

`USoundStyleButton` extends `UButton` with **Sound Style** (the `SoundClasses` tag container, shown as Sound Style so it isn't confused with Unreal Sound Classes; readable and writable from Blueprint).

- It adds its own bindings to `OnHovered`, `OnPressed` and `OnReleased` when its Slate widget is built. Blueprint bindings on the same events keep working.
- **Hold:** Press starts the `Hold` loop and Release stops it. If the button is torn down while held (the screen closes), the loop stops too, because Release never comes.
- **Gamepad:** on `OnReceivedFocus`, it plays Hover only if the button isn't already hovered, so a mouse click (which also focuses) doesn't play a second hover.
- **Focus set by code is quiet:** for about 0.1 s after a screen plays `Open`, focus changes don't play Hover. A screen that focuses its first button on open plays Open, not Open and Hover. UButton's focus event doesn't say why focus moved, so a short window after Open is the reliable option.
- The Button Style's own **Hovered, Pressed and Clicked Sound** slots are hidden in the Details panel (`SoundStylesEditor` module). They're also cleared, with a warning, when the button is built, which covers buttons converted with those slots already filled and styles set from Blueprint. A sound never plays twice.

### Slider

`USoundStyleSlider` extends `USlider` with **Sound Style** and **Step Count**.

- **Grab** (mouse or gamepad capture): Press, then the `Hold` loop, which starts with `Value` already set to the handle's position. **Let go:** Release, and the loop stops.
- **While it moves**, `Value` (the normalized position, 0–1, whatever the slider's Min and Max) goes to the loop, so the sound can follow the handle.
- **Detents:** the range is cut into Step Count slices, and a `Step` one-shot (with `Value`) plays each time the value moves into a new slice. Steps only play when the player is moving it (captured, or focused for the arrow keys). `USlider::SetValue` from code also fires the change event, so this keeps a settings screen loading saved values quiet.

### CheckBox

`USoundStyleCheckBox` extends `UCheckBox`. A player toggle plays `Press` with `Checked` (true = now on), so one MetaSound can sound different for on and off. `Set Is Checked` from code doesn't fire the change event, so it stays silent without any extra check.

### Progress bar

`USoundStyleProgressBar` extends `UProgressBar` with **Sound Style**, **Step Count** and **Quiet Seconds**. `UProgressBar::SetPercent` isn't virtual, so the bar adds **Set Percent With Sound**:

- the first change starts the `Fill` loop, and later changes update its `Value`
- a `Step` one-shot plays at each detent crossed
- the loop stops once the bar hasn't moved for Quiet Seconds (default 0.2 s). A progress bar has no "start" or "stop" event of its own, so this is how it knows.

### Scope interface

`ISoundStyleScopeInterface`, implementable in Blueprint, with one function: **Get UI Sound Classes** (returns a tag container). Screens and wrapper widgets implement it.

### Widget interface

`ISoundStyleWidgetInterface`, C++ only, with one function: `GetOwnSoundClasses`. Each of the plugin's widgets implements it, so the subsystem asks "does this widget carry its own classes?" without knowing every widget type. A new adapter widget implements it and needs no subsystem change.

### Table editor

Double-clicking a Sound Style Table opens a dedicated editor (`SoundStylesEditor` module), instead of the raw Details panel:

- **Rows** (left): one line per class, sorted and indented by depth, with a class tag picker, *(default)* on the root row, and delete. **+ Add Row** adds an empty row whose class is picked in the row.
- **Class** (right): the selected row's While Open Mix and every event, showing the sound that will actually play and where it comes from (this class, inherited from a parent, the default, or not set). Picking a sound overrides it on this class; **Silence** and **Reset** set the event silent or let it inherit again.
- **Parameters** (below the events): the selected event's fixed parameters, as name, type (Float, Int or Bool) and value.
- Every edit is one undo step. A **Details (raw)** tab is available from the Window menu.

### Typewriter

One `Type` sound per letter. The dialogue box skips spaces, and **Skip** completes the text without playing ticks. `SCON_UI`'s retrigger time keeps fast text from machine-gunning.

### Tests

Resolution is pure data, so automated tests build a table in code and check: exact match, parent fallback, the root default, silent stopping resolution, several classes (deepest wins), a row without the event, scope fallback, own class beating scope, and an invalid class. The state stack gets tests for stacking, shared mixes, popping without a push and clearing. All 14 are named `SoundStyles.*`. Playback, loops, parameters, gamepad focus and the settle windows need a world, so they're checked by hand in PIE.

### Later

- A dropdown (ComboBox) adapter, when the options menu needs one.
- Tests for the slider and progress bar detent maths (plain numbers, no world needed).

## Style guide

- **Add an Audio section** listing each sound class with a one-line description of when to use it.
- **Match the Buttons section.** Each button type in the style guide (primary, secondary, back, tab, ...) gets a matching class, so choosing the visual style tells you the sound.

## Current UI and how it maps (review, Nov 2026)

Every widget is a plain `UserWidget` with UMG Buttons, and nothing plays a sound today. CommonUI is enabled (viewport client, input settings, the ballast indicator reads the input type), but no widgets use CommonUI buttons.

| Widget | Plan |
|---|---|
| `WBP_MenuButton` | Inner Button becomes a Sound Style Button with no classes, so it follows each menu's scope |
| `WBP_MainMenu` | Scope `Menu`. New Game: `Menu.Primary.NewGame`. Quit needs a short delay before Quit Game, or its press sound is cut off |
| `WBP_PauseMenu` | Scope `Menu`, state `Pause`, Open/Close sounds. Return to Game: `Menu.Primary`. Return to Main Menu: `Menu.Back` |
| `WBP_CreditsScreen` | Scope `Menu`, Open sound. Back button: `Menu.Back` |
| `WBP_Journal` | Scope `Journal`, state `Journal`, Open/Close in Show/Hide Journal. Tabs: `Journal.Tab` (Press silent, `Select` on tab change). The tab blink plays `Notify` |
| `WBP_JournalInvestigationItem` | `Journal.Item`, set once in the row widget class |
| `WBP_DialogueBox` | `Type` per letter, class from the speaker |
| `WBP_OxygenIndicator`, `WBP_BallastIndicator` | Their alarms and valve sounds are player feedback. Only purely visual flourishes would be UI sounds |
| Debug menu | Plain C++ Slate, not UMG. Dev-only, stays silent |

Two things the review found that affect UI audio:
- `OnInvestigationUpdated` also fires for description-only edits, so the journal should only play Notify when an investigation's **status** changes. A retrigger time on the `Notify` sound's concurrency catches bursts.
- There's no HUD notification widget, so notifications while the journal is closed have nowhere to play from yet.

## Open questions

1. **Plain Button or CommonUI?** This design extends the UMG Button because that's what every widget uses. If UI moves to CommonUI's `CommonButtonBase`, the table, subsystem and scopes stay the same, and only the button adapter changes. Better to decide before widgets are converted.

## Implementation plan

| Step | What | Where | Who | Status |
|---|---|---|---|---|
| 1 | Plugin skeleton (`.uplugin`, runtime module, `LICENSE`, README), root and plugin tags, data types, settings | Plugin repo | Clara | Done |
| 2 | Subsystem: resolution with classes and scopes, Play UI Sound, debug output, tests | Plugin repo | Clara | Done |
| 3 | Scope interface and `USoundStyleButton` (hover, press, release, gamepad focus, settle window) | Plugin repo | Clara | Done |
| 4 | First release copied into `Plugins/SoundStyles` and enabled; `DA_AtlantisUISoundStyles`, class tags, settings entry | This repo | Audio | To do |
| 5 | Convert `WBP_MenuButton` and the menus; first sound pass | This repo | Audio + UI | To do |
| 6 | Screen audio states (stack, clearing on travel) in the plugin; state mixes for pause and journal in the game | Both | Audio | Plugin done; game to do |
| 7 | Journal tabs, rows and Notify | This repo | Audio + UI | To do |
| 8 | Dialogue typewriter | This repo | Audio + UI | To do |
| 9 | Loading suppression (repeat limits use Sound Concurrency on the sounds) | Plugin repo | Clara | Done |
| 10 | Audio section in the style guide | Style guide | Audio + UI design | To do |
| 11 | Table editor; loops (`Hold`, `Fill`); MetaSound parameters; slider, checkbox and progress bar widgets | Plugin repo | Clara | Done |
| Later | Dropdown adapter, detent tests | Plugin repo | Clara | — |
