# UI Audio: Sound Styles

> **Status: proposal.** Nothing here is built yet. Comments welcome before implementation starts. The system will be **Sound Styles**, a reusable MIT-licensed plugin owned by Clara Smith; see [Plugin](#plugin).

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
- **More than one-shots.** A screen can also change the mix while it's open, for example ducking the world under the pause menu.

## Concepts

### Classes and events

| Tag root | Meaning | Examples |
|---|---|---|
| `UISound.Class` | **Class**: what kind of element this is | `UISound.Class.Menu.Primary`, `UISound.Class.Journal.Tab` |
| `UISound.Event` | **Event**: what just happened to it | `UISound.Event.Hover`, `UISound.Event.Press`, `UISound.Event.Open` |

Starting events: `Hover` (mouse over or gamepad focus), `Press`, `Release`, `Open`, `Close`, `Select` (tabs), `Notify`, `Type`. Add more as needed; nothing in code lists them. Tag names below drop the `UISound.Class.` / `UISound.Event.` prefix for readability.

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

If two of a widget's classes set the same event at the same depth, that's ambiguous. The first one listed wins, and the debug output warns about it.

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

1. Add the **Sound Style Scope** interface to the screen widget and return its class from **Get UI Sound Classes**, for example `Menu`.
2. In the screen's open and close handlers, call **Play Widget UI Sound** with `Open` / `Close`, and **Push** / **Pop UI Audio State** if the screen should change the mix.

### UI developers: a button

1. In the widget designer, right-click the Button and choose **Replace With > Sound Style Button**. The slot, style and bindings carry over.
2. Leave **Sound Classes** empty to use the screen's scope. Set it only when the button should sound different.
3. Leave the Button Style's own **Hovered/Pressed Sound** slots empty, or the sound plays twice.

If no existing class fits, ask audio for one rather than adding sounds to the widget.

### Audio: adding or changing a class

1. Add the tag under `UISound.Class` in Project Settings > GameplayTags.
2. Add a row for it in `DA_AtlantisUISoundStyles`, filling only the events that differ from its parent.
3. Make sound presets from `MSS_Stereo_OneShot_lib` (#65). They route to `SC_UI` and `Submix_UI` with high priority.

No code changes and no widget changes.

### Finding the row to edit

Set `SoundStyles.Debug 1` in the console. Each UI sound then logs the widget, its classes and scopes, the event, and which row answered:

```
QuitButton  scope Menu / Press   -> row Menu
NewGame     Menu.Primary.NewGame / Press -> row Menu.Primary.NewGame
JournalTab  Journal.Tab / Press  -> row Journal.Tab (silent)
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
| Meter tick as the oxygen bar animates | UI: from `WBP_OxygenIndicator` |
| Oxygen low or critical alarm, breathing strain | **Player feedback**, from `Audio.Fact.Oxygen.Level` / `Oxygen.Critical` |
| Oxygen refill at a membrane (#80), ballast valve | **Player feedback**, from facts or the gameplay event |

**Player feedback is not part of this system.** It listens to the same audio facts as the music (the fact bridge, #489, already sends the oxygen facts), so the alarm stays audible while the journal, a cutscene or the debug menu hides the HUD, and stays in step with the music. It gets its own design.

## Plugin

The system is built as a plugin, **Sound Styles**, so it has no dependency on game code and can be reused in other projects. The game uses it like any other plugin.

### Ownership and license

Sound Styles is **Clara Smith's own plugin, licensed MIT**, and developed in its own repository (`clarasmith-sound/SoundStyles`), not in this one. ASGC uses it under the MIT license, which is compatible with this project's AGPL-3.0.

- **Its files carry `Copyright (c) 2026 Clara Smith`**, not the ASGC header, and the plugin folder includes its `LICENSE` file.
- **ASGC includes a release copy** in `Plugins/SoundStyles`, not a git submodule, so nobody needs extra clone steps. The plugin's README in that folder records its source repository and version.
- **Changes go upstream first.** Fix or extend the plugin in its own repository, then a PR here copies in the new version. Don't edit `Plugins/SoundStyles` in this repo directly, or the next update will overwrite the change.
- **Contributions are welcome** as PRs to the plugin's repository, under the same MIT license.
- **No ASGC code goes into the plugin.** It's written fresh, so it stays clean to license separately. Game code that uses it (the style table, classes, widget setup, player feedback) stays in this project under AGPL.

### What lives where

| Plugin (`Plugins/SoundStyles`) | Game (ProjectAtlantis) |
|---|---|
| `USoundStyleTable`, `FSoundStyleClass`, `FSoundStyleEvent` (the data types) | `DA_AtlantisUISoundStyles` (the table asset and its rows) |
| `USoundStyleSubsystem` (resolution, playback, state stack, debug output) | Class tags: `UISound.Class.Menu`, `UISound.Class.Journal`, ... |
| `USoundStyleButton` and `ISoundStyleScope` | Game-specific events: `Select`, `Notify`, `Type`, ... |
| `USoundStyleSettings`: **Project Settings > Plugins > Sound Styles**, where the project picks its table | Sound presets (from `MSS_Stereo_OneShot_lib`), state mixes (`CBM_UIState_*`) |
| Root tags `UISound.Class` and `UISound.Event`, plus the events the plugin plays itself: `Hover`, `Press`, `Release`, `Open`, `Close` | Widgets: scopes, classes and Play UI Sound calls |
| Automated tests for resolution and the state stack | Player feedback (not part of this system) |

The rule: **the plugin knows how sounds are chosen and played, the game decides which sounds and where.** Nothing in the plugin names a game class, asset or tag below the roots.

### Structure

```
Plugins/SoundStyles/
  LICENSE                       MIT
  README.md                     source repository, version, usage
  SoundStyles.uplugin           depends on the AudioModulation plugin
  Source/SoundStyles/           runtime module
    Public/   SoundStyleTable.h, SoundStyleSubsystem.h, SoundStyleButton.h,
              SoundStyleScope.h, SoundStyleSettings.h, SoundStyleTags.h
    Private/  ... and Tests/
  Source/SoundStylesEditor/     editor module, added later for the two-pane editor
```

- **Runtime module dependencies:** `Core`, `CoreUObject`, `Engine`, `UMG`, `Slate`, `SlateCore`, `GameplayTags`, `DeveloperSettings`, `AudioModulation`.
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

    // Drops repeats of this class + event closer together than this. 0 = no limit.
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "s"))
    float MinInterval = 0.f;
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

- **Missing event:** inherits. **Event present with no sound:** silent, and resolution stops.
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

The root is excluded from the per-class walks, so a widget's own classes can't hide its scope behind the global default. Scopes are found once when the widget is built, by walking up parent widgets (across nested user widgets) to each widget implementing the scope interface. A class outside `UISound.Class` logs a warning and is ignored.

### Playback

Sounds play through `FSlateApplication::PlaySound`, the same path UMG uses for its built-in button sounds. It plays with no world, so a press sound isn't cut off when the button starts a level load. It's flagged as a UI sound, so it plays while paused, and it uses the asset's priority. `UGameplayStatics::PlaySound2D` would tie the sound to the current world and cut it off on level travel.

All timing (minimum intervals, the focus settle window) uses **real time**, because the journal and pause menu pause the game.

### Subsystem

`USoundStyleSubsystem` is a game instance subsystem, so it lives from the main menu through every level. It:

- loads the table set in **Project Settings > Plugins > Sound Styles** in `Initialize` and keeps it loaded. It logs an error if the table isn't set.
- plays sounds: **Play UI Sound** (classes + event) and **Play Widget UI Sound** (a widget + event, which uses the widget's classes and scopes).
- keeps the **screen audio state stack**, with each mix reference-counted so two screens sharing a mix don't cancel each other.
- **clears every state on level travel.** A screen can open a level without closing first (pause menu > Return to Main Menu), and a state left active would duck the next level.
- **drops UI sounds while a level loads** and for a moment after, so restored journal entries don't chime.
- applies each event's **minimum interval**, keyed by the class row and event that answered.
- does nothing where Slate isn't running (automated tests, commandlets).

### Button

`USoundStyleButton` extends `UButton` with **Sound Classes** (a tag container with a Blueprint setter, so MVVM can bind it).

- It adds its own bindings to `OnHovered`, `OnPressed` and `OnReleased` when its Slate widget is built. Blueprint bindings on the same events keep working.
- **Gamepad:** on `OnReceivedFocus`, it plays Hover only if the button isn't already hovered, so a mouse click (which also focuses) doesn't play a second hover.
- **Focus set by code is quiet:** for about 0.1 s after a screen plays `Open`, focus changes don't play Hover. A screen that focuses its first button on open plays Open, not Open and Hover. UButton's focus event doesn't say why focus moved, so a short window after Open is the reliable option.
- It warns if the Button Style's own Hovered/Pressed Sound slots are set.

### Scope interface

`ISoundStyleScope`, implementable in Blueprint, with one function: **Get UI Sound Classes** (returns a tag container). Screens and wrapper widgets implement it.

### Typewriter

One `Type` sound per letter. The dialogue box skips spaces, and **Skip** completes the text without playing ticks. The `Type` event's minimum interval or `SCON_UI`'s retrigger time keeps fast text from machine-gunning.

### Tests

Resolution is pure data, so automated tests build a table in code and check: exact match, parent fallback, the root default, silent stopping resolution, several classes (deepest wins, tie warns), scope fallback, own class beating scope, and an invalid class. The state stack gets tests for stacking, shared mixes and clearing on travel. Playback, gamepad focus and the settle window are checked by hand in PIE.

### Later

- A two-pane editor (classes on the left, overrides on the right) as an editor utility widget. The data format stays the same.
- Looping events, if per-letter typing or another case needs one.
- Slider, checkbox and dropdown adapters, when the options menu is built.

## Style guide

- **Add an Audio section** listing each sound class with a one-line description of when to use it.
- **Match the Buttons section.** Each button type in the style guide (primary, secondary, back, tab, ...) gets a matching class, so choosing the visual style tells you the sound.

## Current UI and how it maps (review, Nov 2026)

Every widget is a plain `UserWidget` with UMG Buttons, and nothing plays a sound today. CommonUI is enabled (viewport client, input settings, the ballast indicator reads the input type), but no widgets use CommonUI buttons.

| Widget | Plan |
|---|---|
| `WBP_MenuButton` | Inner Button becomes an Sound Style Button with no classes, so it follows each menu's scope |
| `WBP_MainMenu` | Scope `Menu`. New Game: `Menu.Primary.NewGame`. Quit needs a short delay before Quit Game, or its press sound is cut off |
| `WBP_PauseMenu` | Scope `Menu`, state `Pause`, Open/Close sounds. Return to Game: `Menu.Primary`. Return to Main Menu: `Menu.Back` |
| `WBP_CreditsScreen` | Scope `Menu`, Open sound. Back button: `Menu.Back` |
| `WBP_Journal` | Scope `Journal`, state `Journal`, Open/Close in Show/Hide Journal. Tabs: `Journal.Tab` (Press silent, `Select` on tab change). The tab blink plays `Notify` |
| `WBP_JournalInvestigationItem` | `Journal.Item`, set once in the row widget class |
| `WBP_DialogueBox` | `Type` per letter, class from the speaker |
| `WBP_OxygenIndicator`, `WBP_BallastIndicator` | Their alarms and valve sounds are player feedback. Only purely visual flourishes would be UI sounds |
| Debug menu | Plain C++ Slate, not UMG. Dev-only, stays silent |

Two things the review found that affect UI audio:
- `OnInvestigationUpdated` also fires for description-only edits, so the journal should only play Notify when an investigation's **status** changes. The minimum interval on `Notify` catches bursts.
- There's no HUD notification widget, so notifications while the journal is closed have nowhere to play from yet.

## Open questions

1. **Plain Button or CommonUI?** This design extends the UMG Button because that's what every widget uses. If UI moves to CommonUI's `CommonButtonBase`, the table, subsystem and scopes stay the same, and only the button adapter changes. Better to decide before widgets are converted.

## Implementation plan

| Step | What | Where | Who |
|---|---|---|---|
| 1 | Plugin skeleton (`.uplugin`, runtime module, `LICENSE`, README), root and plugin tags, data types, settings | Plugin repo | Clara |
| 2 | Subsystem: resolution with classes and scopes, Play UI Sound, debug output, tests | Plugin repo | Clara |
| 3 | Scope interface and `USoundStyleButton` (hover, press, release, gamepad focus, settle window) | Plugin repo | Clara |
| 4 | First release copied into `Plugins/SoundStyles` and enabled; `DA_AtlantisUISoundStyles`, class tags, settings entry | This repo | Audio |
| 5 | Convert `WBP_MenuButton` and the menus; first sound pass | This repo | Audio + UI |
| 6 | Screen audio states (stack, clearing on travel) in the plugin; state mixes for pause and journal in the game | Both | Audio |
| 7 | Journal tabs, rows and Notify | This repo | Audio + UI |
| 8 | Dialogue typewriter | This repo | Audio + UI |
| 9 | Loading suppression and minimum intervals | Plugin repo | Clara |
| 10 | Audio section in the style guide | Style guide | Audio + UI design |
| Later | Two-pane editor, looping events, slider/checkbox/dropdown adapters | Plugin repo | Clara |
