# Puzzle System: Setup Guide

How to build an environmental puzzle in the editor with the puzzle manager (`BP_PuzzleManager`, C++ class
`AAtlantisPuzzleManager`).

Design source: **[FDS-012 Environmental Puzzles](https://docs.google.com/document/d/1fmCHQ-EkEDH-s6jUCRgHwdz0lBCpzSX_2ThSaRyejxU/edit)**.
The worked example is FDS-012's **Flooded Station**, built as Puzzle 1 in `Maps/Puzzles/L_Puzzle_Test_01`
(`PuzzleManager_FloodedStation`, Outliner folder `Puzzle1`). The other FDS-012 examples are covered in
[Patterns from FDS-012](#patterns-from-fds-012).

> **Status (issue #341):** the C++ system is built and covered by automation tests (`Atlantis.Puzzle.Manager`).
> Still to wire:
> 1. `BP_AtlantisPlayerCharacter` calls **Notify Puzzle Interaction** when E hits something, before **Examine**.
> 2. `BP_DoorFrame` implements **Atlantis Puzzle Object** with the states Locked / Unlocked / Membrane
>    (see [Set Object State](#set-object-state)).
>
> Until 2 is done, the door ignores the puzzle and the Output Log warns that it does not implement the interface.

---

## The idea in 30 seconds

- One **Puzzle Manager** per puzzle. Everything is set up in its **Details (Inspector)** panel.
- The manager has a list of **Tasks**. A task is one thing the player does: turn off the power, repair the line.
- Every task has **three lists**:

| List | Meaning | Runs |
|---|---|---|
| **On Begin** | The state this task's objects are in when the game starts | At game start and on every reset |
| **On Fail** | Feedback for trying the task too early (sound, flash, shove) | Each time the player tries it while it is locked |
| **On Complete** | The state the task switches to and **keeps** | Once, when completed; stays until a reset or the game ends |

- Each list holds entries picked from a dropdown, each with its own settings: **Set Actors Visible**,
  **Set Object State**, **Play Sound**, and so on (all listed in [Actions](#actions)).
- A task can **require** other tasks, picked by name in its **Requires** list. Until they are completed it is locked:
  pressing it runs its On Fail. Requires is the only thing that sets the order; where a task sits in the list never matters.
- The manager also has its own **On Begin** (things no single task owns, like the door being locked) and
  **On Solved** (runs once when every task is completed).

FDS-012's response table maps onto this directly:

| FDS-012 player action | Expected response | In the manager |
|---|---|---|
| No meaningful effect | Neutral feedback | Interacting with something that is not part of an open task does nothing |
| Valid intermediate action | Visible or audible progress | The task's **On Complete** |
| Incorrect attempt | Observable result, no significant punishment | The task's **On Fail**; the puzzle never enters a failed state |
| Puzzle resolved | Meaningful change in the world | The manager's **On Solved** |

---

## What the Details panel looks like

```
PuzzleManager_FloodedStation
├─ Reset On Fail     ☐           wrong attempt restarts the whole puzzle (Music Room style)
├─ On Begin                      the puzzle's own start state
├─ On Solved                     runs once when every task is done
├─ Tasks
│   └─ [n] <Task Name>
│         Task Name
│         Interact With          the actor the player presses E on
│         Requires               locked until these named tasks are completed (dropdown; empty = open from the start)
│         On Begin / On Fail / On Complete
└─ Preview
      Preview Task  [dropdown]   pick a task, then:
      [Show Task Start] [Show Task Completed]   that task's On Begin / On Complete only
      [Show Puzzle Start] [Show Puzzle Solved]  the manager's own On Begin / On Solved
      [Clear Preview]
```

To add a task: click **+** on Tasks and fill in the new entry. To add an entry to one of the lists: click **+** on
the list and pick the action type in the new entry's dropdown.

**Preview** changes only what the editor shows; it never changes what is saved. Only entries that change
how things look (Set Actors Visible, Set Object State) run in the preview; sounds and other one-off entries
are skipped. **Clear Preview** unhides everything.

**Before you start:** what the player interacts with must be interactable. E runs a short trace from the
player's body and only hits actors set up for it, so use a `BP_AtlantisInteractable` with the mesh you want,
not a plain Static Mesh Actor. See *Collision Configuration* in `Docs/Examine Mode.md`.

**The setup: Flooded Station example**

```
On Begin:    Set Object State   Targets: BP_DoorFrame   State: Locked      door shut
On Solved:   Set Object State   Targets: BP_DoorFrame   State: Membrane    water purged, player can pass

[0] PowerOff     Interact With: BP_Interact_Lever
    On Begin:     Set Actors Visible  [BP_Interact_Lever, NS_SmokeStylized_Continuous]  Visible ☑
                  Set Actors Visible  [Lever_Off]                                        Visible ☐
    On Complete:  Set Actors Visible  [BP_Interact_Lever, NS_SmokeStylized_Continuous]  Visible ☐
                  Set Actors Visible  [Lever_Off]                                        Visible ☑

[1] RepairLine   Interact With: BP_Interact_Pipe01   Requires: PowerOff
    On Begin:     Set Actors Visible  [BP_Interact_Pipe01, BP_Interact_Pipe02]  Visible ☑
                  Set Actors Visible  [Line_Fixed]                              Visible ☐
    On Fail:      Play Sound  SFX_ElectricShock (placeholder),  BPA_ShockPushback (custom, see below)
    On Complete:  Set Actors Visible  [BP_Interact_Pipe01, BP_Interact_Pipe02]  Visible ☐
                  Set Actors Visible  [Line_Fixed]                              Visible ☑

[2] PowerOn      Interact With: Lever_Off   Requires: RepairLine
    On Begin:     (none: PowerOff already sets both levers' start state)
    On Complete:  Set Actors Visible  [Lever_Off]                          Visible ☐
                  Set Actors Visible  [BP_Interact_Lever, Button_Powered]  Visible ☑

[3] PressButton  Interact With: BP_Interactable_Button   Requires: PowerOn
    On Begin:     Set Actors Visible  [Button_Powered]  Visible ☐
    On Fail:      Play Sound  SFX_ButtonDenied (placeholder)
```

Things to notice:

- **Each actor's start state is set once.** `Lever_Off` gets its start state in PowerOff's On Begin, so PowerOn's
  On Begin leaves it alone. If two On Begin lists set the same actor, the Output Log warns you.
- **PowerOn presses `Lever_Off`, not `BP_Interact_Lever`.** Hidden actors lose their collision, so once the power
  is off the player can only hit `Lever_Off`.
- **"Nothing requires a specific order immediately"** (FDS-012): the player can touch anything at any time.
  Chaining each task with Requires still fits, because touching things early is an incorrect attempt with feedback
  (On Fail), not a failure.
- **The start state is applied one frame after play begins**, once every actor has run its own BeginPlay, so the
  door or anything else cannot overwrite it.

**Check it:** pick **PowerOff** in **Preview Task**, click **Show Task Completed** (lever off, smoke gone), then
**Show Task Start**. Then **Clear Preview**.

---

## Actions

### Set Actors Visible
Shows or hides actors, rendering and collision together. When shown again, an actor gets back whatever collision
it was placed with. Runs in the preview. The main tool for On Begin / On Complete.

**Include Attached Actors** (on by default) also shows or hides everything attached to the targets: the actors
nested under them in the Outliner. Unlike Unity's `SetActive`, Unreal doesn't hide attached actors with their
parent, so without this, hiding `PowerBox` would leave its attached cable segments floating. Untick it to change
only the listed actors.

### Set Object State
For objects that are more than visible or hidden: a door that can be **Locked**, **Unlocked** or a **Membrane**, a
valve that can be **Closed**, **Half** or **Open**. Each such object **lists its own states**, and you pick one:

```
Set Object State   Targets: BP_DoorFrame   State: [Locked ▾]
```

The **State** dropdown only offers states the target actually has (with several targets, only the states they all
share). When the entry runs, the object is told the state's name and reacts however its Blueprint says. Runs in the
preview; **Clear Preview** puts each object back to its default state (the first one it lists).

**Making an object take part (do this once per object type):**
1. Open the object's Blueprint, e.g. `BP_DoorFrame`.
2. **Class Settings → Implemented Interfaces → Add → Atlantis Puzzle Object.**
3. In **My Blueprint → Interfaces**, double-click **Get Puzzle States** and return the list of state names, default
   first. For the door: `Locked`, `Unlocked`, `Membrane`. These names are exactly you'll see in the dropdown.
4. Right-click the graph and add **Event On Puzzle State (State, Is Preview)**. Connect it to a **Switch on Name** node
   with one case per state, and do what each state means:
   - `Locked` → set a `Locked` Boolean to true (the door's overlap → **Open Door** path checks it first).
   - `Unlocked` → set `Locked` to false.
   - `Membrane` → set `Locked` to false and swap to the membrane material or mesh.
   **Is Preview** is true when a designer clicks a preview button in the editor: change only what should be seen,
   skip sounds.
5. Compile and save. The object's states now appear in every Set Object State dropdown that targets it.

If an entry targets an object that doesn't implement the interface, has no state picked, or picks a state the object
doesn't list, the Output Log warns you when you edit the puzzle and when play starts.

### Play Sound
Plays a sound at **Play At**, or at the task's Interact With actor if empty, or at the puzzle manager when there is
neither (the manager's own lists, tasks without an Interact With), so place the manager at the puzzle. **Play 2D**
ignores location. Use the project's sound classes (`SC_SFX`) and attenuation (`ATT_Default`). Skipped in the
preview.

### Set Components Active
Activates or deactivates components on actors; defaults to effects (Niagara). Activating restarts the effect, so a
burst plays once each time; deactivating lets existing particles finish. Useful in On Solved for a purge burst
(`NS_ToonSteam_Burst` with **Auto Activate** off). Skipped in the preview.

### Complete Puzzle Task
Completes a task on another puzzle manager, by **Task Name**. See Drowned Pumpworks below. Skipped in the preview.

### Custom action in Blueprint: shock and push back
For RepairLine's On Fail. Content Browser → **Blueprint Class** → **All Classes** → **Atlantis Puzzle Action**,
named `BPA_ShockPushback` *(new asset)*. Add instance-editable floats `Strength` (800) and `Up` (300). Override
**Execute (Context)**:

```
Event Execute (Context)
  → Break Atlantis Puzzle Action Context → Instigator, Task Actor
  → Cast To Character (Instigator)
  → Direction = normalize(Instigator location − Task Actor location)
  → Launch Character (Direction × Strength + (0, 0, Up), XY Override, Z Override)
```

It then appears in every list's dropdown. **Context** carries `Manager`, `TaskIndex` (-1 for the manager's own
lists), `TaskActor` (the task's Interact With), `Instigator` (whoever pressed E; empty when the puzzle triggered
itself) and `Is Preview`. Leave **Run In Preview** off on custom actions unless they only change how things look; if you turn it on, also
override **Clear Preview** so the manager's Clear Preview button can undo it.
There is no player health system yet, so the shock only pushes.

### Custom action in C++
Derive from `UAtlantisPuzzleAction` and override `Execute_Implementation`. Set `bRunInPreview = true` in the
constructor only for look-only actions (and then override `ClearPreview_Implementation` to undo the preview), and override `GetAffectedActors` if the action sets actors' state (that feeds the On Begin warning). Any such class shows up in the dropdown automatically.

---

## Patterns from FDS-012

### Flooded Maintenance Station: two conditions, any order
Pull the displaced mechanism into line and restart the generator, in either order; the gate opens when both are
done. Leave both tasks without requirements (available from the start), and put the gate in On Solved:

```
[0] RealignMechanism   Interact With: Mechanism_Displaced   On Complete: displaced → aligned
[1] RestartGenerator   Interact With: Generator_Controls    On Complete: generator off → running
On Solved: Set Object State  Gate  State: Open
```

To make the gate itself a task the player presses after both: add `[2] OpenGate` with **Requires:
RealignMechanism, RestartGenerator**.

### Music Room: the order matters and a mistake starts over
Press four buttons in the order shown by the paintings. A wrong button makes the walrusfolk sneeze and every button
returns to neutral.

```
Reset On Fail: ☑
[n] Button_n   Requires: Button_(n-1)  (empty for the first)
    On Begin:     Button_n_Pressed hidden
    On Fail:      Play Sound (sneeze)
    On Complete:  Button_n_Pressed shown
On Solved: the song, the walrusfolk animation (custom action, or Blueprint bound to On Puzzle Solved)
```

**Reset On Fail** runs the wrong button's On Fail, then resets the whole puzzle (every On Begin runs again). A
pressed button can't be pressed again until then.

### Drowned Pumpworks: sections that feed a main puzzle
Build each section as its own manager and chain them with **Complete Puzzle Task**:

```
Engine Hall manager
  [0] DriveRealigned   Interact With: (empty: only completed by the Lower Works)
  [1] PowerRestored    Interact With: (empty: only completed by the Upper Relay)
  [2] ActivateGate     Requires: DriveRealigned, PowerRestored
Lower Works manager
  ...its own tasks (clear debris, pull the assembly)...
  On Solved: Complete Puzzle Task   Puzzle: Engine Hall manager   Task Name: DriveRealigned
```

A task with no Interact With can only be completed by Complete Puzzle Task (or `TryCompleteTask` from Blueprint).
If the target task is still locked, it stays locked.

### Repeatable puzzles
Call **Reset Puzzle** from On Solved (a custom action) or from a Blueprint bound to **On Puzzle Solved**. The reset
waits until the solve has finished. Reset Puzzle called from an **On Begin** is ignored with a warning, because it
would reset forever.

### Not covered yet
- **Choosing between positions** (Pumpworks drive positions A/B/C). Needs a multi-state selector.
- **Quantities** (the boiler that needs exactly 5 litres).
- **Physical and held objects**, sliding tiles, and Equipment (Corded Harpoon, Shock Tip). Those systems can still
  report to a puzzle: when the harpoon pulls an actor or the Shock Tip powers it, call **Notify Puzzle
  Interaction** with that actor and it completes the task like pressing E.

---

Code reference: [`AtlantisPuzzleManager.h`](AtlantisPuzzleManager.h), [`AtlantisPuzzleTypes.h`](AtlantisPuzzleTypes.h),[`AtlantisPuzzleActions.h`](AtlantisPuzzleActions.h), [`AtlantisPuzzleObject.h`](AtlantisPuzzleObject.h).
