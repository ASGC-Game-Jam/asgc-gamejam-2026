# Sound Styles

UI sounds for Unreal Engine that work like CSS.

You tag widgets with **sound classes** (gameplay tags), and one **style table** decides what each class sounds like. UI developers never pick sound assets, and audio can re-skin every button in the game by editing one table.

```
WBP_PauseMenu          scope: Menu
  ResumeButton         classes: Menu.Primary
  BackButton           classes: Menu.Back
  QuitButton           (none: uses the screen's Menu scope)
```

- **Inheritance:** a class only lists what's different from its parent tag.
- **Scopes:** a screen sets its class once, and every widget inside it picks it up.
- **Gamepad included:** moving focus plays Hover.
- **Game data drives the sound:** sliders, checkboxes and progress bars send their state to MetaSound inputs.
- **Screen audio states:** a screen can activate a Control Bus Mix while it's open, for example to duck the world under the pause menu.

Engine: **Unreal Engine 5.7**. License: **MIT** (see [LICENSE](LICENSE)).

## Contents

- [Install](#install)
- [Quick start](#quick-start)
- [How a sound is chosen](#how-a-sound-is-chosen)
- [Widgets](#widgets)
- [Events](#events)
- [Game data and MetaSound parameters](#game-data-and-metasound-parameters)
- [Screen audio states](#screen-audio-states)
- [Blueprint reference](#blueprint-reference)
- [Behaviour you can rely on](#behaviour-you-can-rely-on)
- [Debugging](#debugging)
- [Tests](#tests)

## Install

1. Copy this folder to `YourProject/Plugins/SoundStyles`.
2. Open the project and enable **Sound Styles** in **Edit > Plugins**. It also enables **Audio Modulation**, which it depends on.
3. If your game's C++ calls the plugin, add `"SoundStyles"` to your module's dependencies. Blueprint-only projects don't need this.

## Quick start

1. **Make a table.** In the Content Browser: **right-click > Audio > Sound Styles > Sound Style Table**.
2. **Point the project at it.** In **Project Settings > Game > Sound Styles**, set **Style Table**.
3. **Set the defaults.** Open the table (it opens when you create it). The editor shows rows on the left and the selected row's events on the right. A new table already has the root row, `UISound.Class`, shown as *(default)*. Pick sounds for its **Hover** and **Press**: every widget falls back to these.
4. **Use the widgets.** In the UMG Palette, open the **Sound Styles** category and use **Sound Style Button**, **Slider**, **CheckBox** or **Progress Bar**.
   To convert an existing Button: select **Sound Style Button** in the Palette (click it once, don't drag it). Then right-click the Button in the Hierarchy and choose **Replace With... > Replace With SoundStyleButton**. The slot, style and bindings carry over.
5. **Play it.** In PIE, hovering and pressing the buttons plays the default sounds.

From here, add classes for widgets that should sound different (see below).

## How a sound is chosen

### Classes and events

| Tag root | Meaning | Example |
|---|---|---|
| `UISound.Class` | **what kind of element** this is | `UISound.Class.Menu.Back` |
| `UISound.Event` | **what just happened** to it | `UISound.Event.Press` |

Add your classes under `UISound.Class` in **Project Settings > GameplayTags**, or with the **+** in the table editor's class picker. Each row of the table is one class, and lists the events it sets.

### Inheritance

Each event inherits from the parent class unless the class sets it. The root row (`UISound.Class`) is the game-wide default.

```
(root)            Hover: tick     Press: click
└─ Menu                           Press: thunk
   └─ Back                        Press: soft close
```

A button with class `Menu.Back` plays *soft close* on Press (set on `Menu.Back`) and *tick* on Hover (nothing sets it below the root).

An event can be **silent**: listed on the row with no sound. That stops it inheriting. In the table editor, use **Silence**. **Reset** removes the event from the row, so it inherits again.

### Several classes on one widget

A widget can have more than one class. For each event, the **deepest** class that sets it wins. If two classes at the same depth both set it, the first one listed wins. With `SoundStyles.Debug 1`, a warning is logged once for each pair of rows.

### Scopes

A screen, or any widget that contains others, can be a **scope**:

1. In the Widget Blueprint, open **Class Settings** and add the interface **Sound Style Scope Interface**.
2. Implement **Get UI Sound Classes** and return the screen's classes, for example `UISound.Class.Menu`.

The lookup order for one event is:
1. the widget's own **Sound Style** property
2. the nearest scope above it, then the next scope up, and so on (this crosses into parent Widget Blueprints)
3. the root row

So a button with no classes of its own sounds like its screen, and a button with its own class overrides the screen. Wrapper widgets (for example a `WBP_MenuButton` with a Sound Style Button inside) need nothing extra: the inner button finds the scope itself.

## Widgets

All four are in the Palette under **Sound Styles**. Each has a **Sound Style** property: leave it empty to use the screen's scope.

| Widget | Plays | Sends |
|---|---|---|
| **Sound Style Button** | Hover (mouse over or gamepad focus), Press, Release, and the **Hold** loop while pressed | — |
| **Sound Style Slider** | Press when grabbed, the **Hold** loop while dragging, a **Step** each time it crosses a detent, Release when let go | `Value` (0–1) to the loop and the steps |
| **Sound Style CheckBox** | Press when the player toggles it | `Checked` (true = now on) |
| **Sound Style Progress Bar** | the **Fill** loop while it moves (stops after **Quiet Seconds** without a change), a **Step** at each detent | `Value` (0–1) |

Details:
- **Button:** the Button Style's own Hovered/Pressed/Clicked Sound slots are hidden. If one was already set, it's cleared and a warning explains why, so a sound never plays twice.
- **Slider and Progress Bar:** **Step Count** sets the detents (10 = a step every 10%, 0 = no steps).
- **Slider:** steps only play when the player moves it (dragging, or the arrow keys while it's focused). The game setting the value stays silent.
- **CheckBox:** **Set Is Checked** from code stays silent.
- **Progress Bar:** call **Set Percent With Sound** to move it. Plain **Set Percent** stays silent.
- **Hover:** only the Button plays Hover (mouse over or gamepad focus). The Slider, CheckBox and Progress Bar don't play Hover.

Other widgets can play sounds through the subsystem with **Play Widget UI Sound** (see the [Blueprint reference](#blueprint-reference)).

## Events

The plugin defines these. Add your own directly under `UISound.Event`: the table editor lists them automatically.

| Event | Played by | Kind |
|---|---|---|
| `Hover` | Button | one-shot |
| `Press` | Button, Slider (grab), CheckBox | one-shot |
| `Release` | Button, Slider (let go) | one-shot |
| `Open`, `Close` | your screens, with **Play Widget UI Sound** | one-shot |
| `Hold` | Button (while pressed), Slider (while dragging) | **loop**: the sound asset must loop |
| `Step` | Slider, Progress Bar (each detent) | one-shot |
| `Fill` | Progress Bar (while moving) | **loop**: the sound asset must loop |

To limit fast repeats, for example sweeping the mouse across a list, set **Sound Concurrency** on the sound (Retrigger Time, Max Count). The plugin has no rate limit of its own.

## Game data and MetaSound parameters

Sounds can react to game data through MetaSound inputs. Parameters are the engine's `FAudioParameter` (name, type and value). In Blueprint, make them with **Float To Audio Parameter** (or **Make AudioParameter**), then put them in an array.

**Names must match the MetaSound's inputs exactly.** A MetaSound ignores names it doesn't have, without an error. Turn on [debugging](#debugging) to see what was sent.

### Built-in parameters

| Name | Type | Sent by |
|---|---|---|
| `Value` | Float, 0–1 | Slider (handle position), Progress Bar (percent) |
| `Checked` | Bool | CheckBox |

For example, wire `Value` through **Map Range** to a pitch or filter cutoff, so the slider's tone and steps rise as it goes up.

### Sending your own

| Call | Use it for |
|---|---|
| **Play Widget UI Sound With Parameters** | a one-shot that starts with these values (e.g. a score tick whose pitch follows the score) |
| **Start UI Loop With Parameters** | a loop that starts with these values |
| **Set UI Loop Parameters** | updating a playing loop (call it as often as you like, e.g. every Tick) |

### Fixed parameters in the table

Each event in the table can also have **Parameters** of its own. Select the event in the table editor and edit them in the panel underneath. Use these for per-class settings, for example one MetaSound for every button, with `Brightness = 0.2` on Menu and `0.8` on Back.

The table's parameters are sent first, then the game's. **Where both use the same name, the game's value wins.**

## Screen audio states

A class row can have a **While Open Mix** (a Control Bus Mix). It's active while a screen of that class is open:

- **Push UI Audio State** (class) when the screen opens.
- **Pop UI Audio State** (same class) when it closes.

States stack, and each mix is reference-counted, so two screens sharing a mix don't cancel each other. All states are cleared when a level starts loading, because a screen can travel without closing first (for example "Return to Main Menu").

Keep state mixes separate from the player's volume settings mix. When two active mixes set the same bus, Audio Modulation multiplies them, so a state mix scales whatever the player chose.

## Blueprint reference

All on **Get SoundStyleSubsystem** (a Game Instance Subsystem, alive from the main menu through every level).

| Node | What it does |
|---|---|
| **Play UI Sound** (Classes, Event) | plays the event for these classes, with no widget |
| **Play Widget UI Sound** (Widget, Event) | plays the event using the widget's own classes, then its scopes |
| **Play Widget UI Sound With Parameters** | the same, with parameters |
| **Start UI Loop** / **Start UI Loop With Parameters** (Widget, Event) | starts a widget's loop (one per widget; starting another replaces it) |
| **Set UI Loop Parameters** (Widget, Parameters) | updates the widget's playing loop. Does nothing if none is playing |
| **Stop UI Loop** (Widget) | stops the widget's loop with a short fade. Safe if none is playing |
| **Stop All UI Loops** | stops every loop |
| **Push / Pop UI Audio State** (Class) | activates and deactivates the class's While Open Mix |
| **Clear All UI Audio States** | deactivates every state |

`Widget` defaults to Self, so inside a Widget Blueprint you can usually leave it unconnected.

## Behaviour you can rely on

- **Plays while paused and across level loads.** One-shots go through Slate, the same path UMG uses for its own button sounds, so they need no world. A press that starts a level load isn't cut off.
- **Loops are always stopped.** Releasing, letting go, the widget being destroyed, or a level loading all stop a widget's loop. The progress bar's quiet countdown runs on the engine's core ticker, so it also works in pause menus.
- **Quiet while loading.** UI sounds are dropped while a level loads and for 0.5 s afterwards, so restored UI doesn't chime.
- **Focus set by code is quiet.** For 0.1 s after a screen plays **Open**, focus changes don't play Hover. A screen that focuses its first button on open plays Open, not Open and Hover.
- **No table, no crash.** If no table is set, nothing plays and an error is logged at startup.

## Debugging

Type `SoundStyles.Debug 1` in the console. Each UI sound then logs the widget, the event, which row answered, and any parameters sent:

```
LogSoundStyles: SSB_Back / UISound.Event.Press -> row UISound.Class.Menu.Back
LogSoundStyles: VolumeSlider / UISound.Event.Hold (loop) -> row UISound.Class.Menu
LogSoundStyles:     parameters from game: Value=0.80
```

Filter the Output Log by `LogSoundStyles`. Updates to a playing loop aren't logged, because they can happen every frame.

## Tests

The lookup and the state stack are covered by automation tests named `SoundStyles.*` (Resolve, States, Tags). Run them from **Tools > Session Frontend > Automation**, or from the command line with `-ExecCmds="Automation RunTests SoundStyles"`.

## Contributing

Issues and pull requests are welcome at [github.com/clarasmith-sound/SoundStyles](https://github.com/clarasmith-sound/SoundStyles), under the MIT license.
