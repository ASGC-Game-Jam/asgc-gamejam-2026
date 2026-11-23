# Guard Detection

> A Far Cry-style detection meter: an enemy doesn't spot you instantly. While it can see you, a meter fills (faster
> when you're close). Past a threshold it stops and watches you; when the meter is full it "attacks" (Behaviour TBD). Break line of sight
> and it goes looking for you.
>
> Related docs: [Guard Patrol & Search.md](./Guard%20Patrol%20%26%20Search.md),
> [Swimming AI Navigation.md](./Swimming%20AI%20Navigation.md), [HidingSpots.md](./HidingSpots.md).

## Overview

```
AI Perception (sight)                         on the enemy controller
  └─ UAtlantisDetectionComponent              on the enemy controller, ticks by itself
       ├─ Atlantis::AI::Perception::*         pure rules (AtlantisPerceptionModel.h, unit tested)
       ├─ AAtlantisEnemyController::SetStatus / SetDetection
       │     ├─ sight cone colour, OnStatusChanged (Blueprint)
       │     └─ UAtlantisDetectionMarkerComponent → WBP_DetectionMarker (above-head bar + '?' / '!')
       └─ "Atlantis Detection" State Tree evaluator → ST_Enemy (Status, LastKnownLocation, SuspectActor, Suspicion)
```

Code: `Source/ProjectAtlantis/{Public,Private}/AI/Detection/` (`AtlantisDetectionComponent`, `AtlantisPerceptionModel`,
`AtlantisDetectionMarkerComponent`), `AI/StateTree/AtlantisStateTreeDetectionEvaluator`, and `Public/UI/AtlantisDetectionMarkerWidget.h`.

## Setup

1. The enemy's **AI controller** needs an **AI Perception** component with a Sight sense (the enemy controllers
   already have one; sight range is set there, e.g. Sight Radius 1200 / Lose Sight Radius 1500 on
   `BP_PuzzleGuardController`).
2. Add an **Atlantis Detection** component (`UAtlantisDetectionComponent`) to the same controller Blueprint and set
   **Sight Distance Curve** (use `/Game/ProjectAtlantis/Core/AI/SightDistanceCurve`). Without the curve, or without AI Perception, detection is off
   and an error is logged once. Adding it to anything that isn't an AI controller triggers an ensure.
3. The State Tree uses the **Atlantis Detection** global evaluator to read the results.
4. Optional: add an **Atlantis Detection Marker** (`UAtlantisDetectionMarkerComponent`) to the enemy pawn for the above-head UI.

## How the meter works

Each enemy keeps **one meter per player** (multiplayer-ready). Every tick:

1. **Seen players fill their meter.** For each player character the sight sense currently perceives, the meter goes
   up by `SightDistanceCurve(distance) × DeltaTime`, capped at 1. Distance is **3D**: a player above or below the
   enemy counts as far away. There is **no instant detection**, not even at point-blank range. The last known
   location is refreshed every tick the player is seen.
2. **Unseen players** first get a **grace time** (5 s) during which the meter holds; after that it drains at
   **Decay Per Second** (0.1/s).
3. **Aggro** happens when the meter reaches 1. It lasts while the player is seen and through the grace time after
   losing them; it ends when the grace time runs out.
4. **Search clock.** The moment a suspicious (meter ≥ threshold) or aggroed player is lost, a search timer starts at
   **Max Search Duration** (45 s). It only counts down while the player stays unseen.
5. **Re-sight boost.** Seeing a player again while their search is still running lifts their meter to at least the
   threshold, so the enemy goes straight back to Alerted instead of building suspicion from zero.
6. Players with an empty meter, no aggro and no search running are forgotten.

### Sight Distance Curve

X = 3D distance in cm, Y = meter gained **per second** while seen (1 = fully detected in one second). Current
`SightDistanceCurve` keys:

| Distance | Fill rate | Time to Alerted (0.3) | Time to Combat (1.0) |
|---|---|---|---|
| 0 – 500 cm | 0.5 /s | 0.6 s | 2 s |
| 1200 cm | 0.25 /s | 1.2 s | 4 s |
| 1500 cm | 0.2 /s | 1.5 s | 5 s |

The curve is shared by every enemy that uses it, so tuning it changes the whole game. When re-importing it from CSV,
don't add a header row (it eats the first key).

### Hiding

A player whose `UAtlantisHidingComponent::IsHiding()` is true counts as **unseen even in plain line of sight**. A
hidden player can't be discovered or re-discovered. If an enemy already had a meter on you when you hid, it behaves as
if it lost sight of you: grace, then drain, and — if it was suspicious — it searches your last known location (where
you hid). Hiding is also the seam (`IsConcealed`) for future concealment rules.

## Status

The enemy's status comes from its most important player:

| Status | When |
|---|---|
| **Combat** | Any player is aggroed (meter full, or still in the grace time after losing them). |
| **Alerted** | A player is **seen** with meter ≥ **Suspicion Threshold** (0.3). |
| **Searching** | A player is **unseen** with meter ≥ threshold, or their search clock is still running (max 45 s). |
| **Idle** | Otherwise (including a brief glimpse below the threshold). |

Priority: Combat > Alerted > Searching > Idle. The **suspect** (the player the enemy reacts to) is chosen the same way:
aggroed first, then seen & suspicious, then lost & searched for; ties go to the fuller meter, then the most recently
seen player.

Typical flow: patrol (Idle) → glimpse, meter fills → **Alerted** (stops and watches) → meter full → **Combat**
(chases) → you break line of sight → still Combat for the 5 s grace time → **Searching** → the Give Up Search task (or
the 45 s cap) forgets you → **Idle**, back to patrol.

### Tuning (on the component)

| Property | Default | |
|---|---|---|
| Sight Distance Curve | — (set to `SightDistanceCurve`) | Fill rate per second over 3D distance. |
| Suspicion Threshold | 0.3 | Meter level where the enemy reacts (Alerted while seeing, Searching after losing). |
| Max Search Duration | 45 s | Longest search after losing a suspicious player. |
| Grace Time | 5 s | How long the meter holds (and aggro lasts) after losing sight. |
| Decay Per Second | 0.1 | Drain rate after the grace time. |

### API

`GetStatus()`, `GetMeter()` (suspect's meter), `GetSuspect()`, `GetLastKnownLocation(Out)`, `ForgetLostTargets()`
(forgets every player not seen right now, status recomputed immediately — used by Give Up Search), `BeginSearch`,
`MarkSearchPointVisited`, `GetSearchMemory()`, `GetPlayers()`, and `UAtlantisDetectionComponent::FindFor(ControllerOrPawn)`.
Blueprint event **On Status Changed** (Old, New) fires on every status change.

## Driving the enemy controller

If the component's owner is an `AAtlantisEnemyController`, every tick it calls the controller's existing
`SetStatus(Status)` and `SetDetection(Meter)`. So everything that already listened to the controller keeps working:

- the controller's **On Status Changed** Blueprint event,
- the sight cone colour (green Idle, yellow Alerted and Searching, red Combat),
- **On Detection Changed** (Meter, Status), broadcast when the meter moves by more than 0.01, reaches exactly 0 or 1,
  or the status changes,
- the above-head marker.

## Above-head marker

`UAtlantisDetectionMarkerComponent` is a widget component (screen space, drawn at desired size, 130 cm above the pawn
origin, no collision) that you add to the **enemy pawn** and set **Widget Class** to `WBP_DetectionMarker`
(`/Game/ProjectAtlantis/UI/`). The controller finds it on possess and pushes every change; nothing else to wire. With
**Hide When Calm** (default on) it's hidden while the enemy is Idle with an empty meter.

`WBP_DetectionMarker` derives from `UAtlantisDetectionMarkerWidget` and needs no graph. The C++ drives two optional
widgets by name:

- **FillBar** (Progress Bar): fill = meter (always full in Combat), fill colour = status colour.
- **StatusText** (Text Block): `?` while Alerted or Searching, `!` in Combat, empty while Idle; same colour.

| Status | Colour property | Default |
|---|---|---|
| Idle | Idle Color | white (1, 1, 1, 0.9) |
| Alerted | Alerted Color | yellow (1, 0.85, 0) |
| Searching | Searching Color | orange (1, 0.5, 0) |
| Combat | Combat Color | red (1, 0.1, 0.05) |

Layout used: Vertical Box root → StatusText, then a Size Box (80 × 10) holding FillBar. Don't use a Canvas Panel as
root: with "draw at desired size" it would be 0 × 0. Override **On Detection Updated** (Meter, Status) in the widget
for extra effects.

## State Tree evaluator: "Atlantis Detection"

`FAtlantisStateTreeDetectionEvaluator` is a thin reader of the controller's detection component. It holds no
detection logic. Outputs (bind them anywhere in the tree):

| Output | Type | |
|---|---|---|
| Status | EMonsterStatus | Idle when the component is missing. |
| LastKnownLocation | Vector | Where the suspect was last seen. Only meaningful when SuspectActor is set. |
| SuspectActor | Actor | The player the enemy reacts to, or null. |
| Suspicion | float | Suspect's meter (0–1). |

If the controller has no detection component, an error is logged on tree start and the enemy stays Idle. See
[Guard Patrol & Search.md](./Guard%20Patrol%20%26%20Search.md) for how ST_Enemy uses these.

## Debugging

- **Console:** `Atlantis.AI.Detection.Draw 1` draws each enemy's last known locations, its status and meter as text,
  and its search points. `0` turns it off.
- **Visual Logger** (Tools → Debug → Visual Logger): every tick the component logs, per remembered player, a point at
  the last known location (red = aggroed, yellow = seen, orange = lost) with "meter, seen, grace, search", plus a line
  for every status change ("Detection status … -> …"). Category `LogAtlantisAI`; the per-player points are Verbose.
- **Marker:** the above-head bar is the quickest check that the meter fills only while you're seen.
- **Tests:** `Atlantis.AI.Perception.Model` (`Private/Tests/AtlantisPerceptionModel.spec.cpp`) covers 3D distance, no
  instant detection, aggro at 1, the threshold, grace → search, the search clock, re-sight boost, ranking and
  forgetting. See [Automated Tests.md](./Automated%20Tests.md).

## Relationship to the old Perception Evaluator

[Perception.md](./Perception.md) describes the original **Perception Evaluator** (`FStateTreePerceptionEvaluator`):
detection logic inside a State Tree evaluator, 2D distance, an instant-aggro radius, hiding ignored. It **stays in the
code** unchanged so nothing else breaks, but ST_Enemy no longer uses it: its global evaluator is swapped for
**Atlantis Detection**, which has the same output names (Status, LastKnownLocation, SuspectActor, Suspicion).

Differences: detection now lives on a component that ticks on its own (not in the tree), distance is 3D, there's no
instant aggro, hiding makes you unseen, Alerted is driven by the 0.3 threshold, and Searching (with the 45 s cap)
exists. Tuning moved from the evaluator's parameters to the component. Don't run both on the same enemy: each would
push its own status to the controller.

The old evaluator's per-player data (`PerceivedActors` on the controller) is no longer filled by the new system, so
tools that read it (e.g. the existing gameplay debugger category) don't show the new meter.
