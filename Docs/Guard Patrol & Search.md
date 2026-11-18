# Guard Patrol & Search

> How swimming guards patrol a route, and how they search for a player they lost. All behaviour is authored in the
> shared State Tree **ST_Enemy** (`Content/ProjectAtlantis/Core/AI`).
>
> Related docs: [Swimming AI Navigation.md](./Swimming%20AI%20Navigation.md) (Swim To, nav graph),
> [Guard Detection.md](./Guard%20Detection.md) (status, meter, evaluator outputs).

## Patrol routes

### The route: BP_PatrolPath

A route is any actor implementing the **AtlantisPatrolRoute** interface (`IAtlantisPatrolRoute`, Blueprint
implementable). `BP_PatrolPath` implements it:

| Interface function | BP_PatrolPath returns |
|---|---|
| Get Route Spline | its `PatrolSpline` component |
| Get Wait Time At Point (Point Index) | its `DefaultWaitTime` (same wait at every point) |

The **spline points are the patrol stops** (world space). Only the points matter; the curve between them doesn't — the
guard swims point to point with the nav graph (see Swim To).

- **Closed loop** spline → the guard loops: 0, 1, 2, …, last, 0, 1, …
- **Open** spline → the guard ping-pongs: 0, 1, …, last, last−1, …, 0, 1, …
- 1-point route: the guard stays on that point. 2-point open route: alternates between them.

Placing a route: drag `BP_PatrolPath` into the level, edit its spline points, set **Default Wait Time**. Every spline
point must be **in the water** and reachable through the nav graph (keep it at least one capsule height below the
surface and clear of geometry, like a nav node).

### The guard: UAtlantisPatrolComponent

Add an **Atlantis Patrol** component (`UAtlantisPatrolComponent`) to the guard pawn and, **per placed instance**, set
**Patrol Route** to the route actor in the level (only actors implementing the interface can be picked). Two guards can
share a route or use their own.

The component only remembers where the guard is along the route (**Cursor**: point index + direction, visible on the
instance while playing). It has no movement logic; State Tree tasks pick points and move the guard:

- `HasValidRoute()` — route set, implements the interface, has at least one point. An actor that doesn't implement
  the interface logs an error once.
- `SelectPoint(Closest Point | Next Point)` — **Closest Point** picks the route point nearest the guard in 3D
  (starting or resuming a patrol); **Next Point** advances along the route (loop / ping-pong).
- `GetCurrentPointLocation`, `GetCurrentWaitTime` (never negative).

### The Select Patrol Point task

**Select Patrol Point** (`FAtlantisStateTreeSelectPatrolPointTask`):

| Property | Category | Default | |
|---|---|---|---|
| Actor | Context | — | The patrolling pawn (bound automatically). |
| Selection | Parameter | Next Point | Closest Point or Next Point. |
| Point Location | Out (property ref) | — | Bind to the Patrol state's `PointLocation` parameter. |
| Wait Time | Out (property ref) | — | Bind to the Patrol state's `WaitTime` parameter. |

It writes the chosen point and its wait time into the bound **state parameters** of the Patrol state, so the sibling
states (Swim To, Delay) can read them. Succeeds immediately; fails if the pawn has no valid route.

## ST_Enemy layout

Contexts: Actor + AIController. Schema **Scheduled Tick Policy = Denied** (see
[Swimming AI Navigation.md](./Swimming%20AI%20Navigation.md#st_enemy-scheduled-tick-policy-must-be-denied)). Global
evaluator **Atlantis Detection**. Global parameter **SearchSweepCount** (Int, 3).

The status-driven states sit under Root in priority order. Each one enters only for its status and leaves through an
`OnTick` transition back to Root as soon as the status changes, so Root re-picks the right branch.

```
Root
├─ Combat     enter: Status == Combat
│             tasks: Set Focus (Focus Target ← SuspectActor)
│                    Swim To (Target ← LastKnownLocation, Acceptance Radius 150, Repath Distance 200, Follow Target ✓)
│             OnTick: Status != Combat → Root
├─ Alerted    enter: Status == Alerted
│             tasks: Set Focus (Focus Target ← SuspectActor)          (no movement: the guard stops and hovers)
│             OnTick: Status != Alerted → Root
├─ Searching  enter: Status == Searching
│             parameter: SearchPoint (Vector)
│             tasks: Begin Search (Origin ← LastKnownLocation)
│             OnTick: Status != Searching → Root
│  ├─ GoToLKL         Swim To (Target ← LastKnownLocation, r 100)          On Completed → LookAround
│  ├─ LookAround      Look Around                                          On Completed → FindSweepPoint
│  ├─ FindSweepPoint  Find Search Point (Max Sweeps ← SearchSweepCount,
│  │                  Search Point → Searching.SearchPoint,
│  │                  Source = Swim Nav Graph Nodes)                       On Succeeded → GoToSweepPoint · On Failed → GiveUp
│  ├─ GoToSweepPoint  Swim To (Target ← Searching.SearchPoint, r 100)      On Completed → LookAround
│  └─ GiveUp          Give Up Search                                       On Completed → Patrol (safety net)
└─ Patrol     (no enter condition: the fallback)
              parameters: PointLocation (Vector), WaitTime (Float)
              OnTick: Status != Idle → Root
   ├─ ResumePatrol     Select Patrol Point (Closest Point → params)   Succeeded → GoToPatrolPoint · Failed → NoRoute
   ├─ GoToPatrolPoint  Swim To (Target ← Patrol.PointLocation)        Succeeded → WaitAtPoint · Failed → RetryDelay
   ├─ WaitAtPoint      Delay (Duration ← Patrol.WaitTime)             Completed → AdvancePatrol
   ├─ RetryDelay       Delay 1 s                                      Completed → AdvancePatrol
   ├─ AdvancePatrol    Select Patrol Point (Next Point → params)      Succeeded → GoToPatrolPoint · Failed → NoRoute
   └─ NoRoute          Delay (Run Forever)
```

"Delay" is the engine's **Delay Task**. All status values and locations come from the **Atlantis Detection** evaluator
outputs.

### Patrol

1. **ResumePatrol** picks the route point closest to the guard (so after a chase or search it resumes from wherever it
   ended up, not from where it left).
2. **GoToPatrolPoint** swims there. If the move fails (no path, blocked) **RetryDelay** waits 1 s and the guard tries
   the next point instead of getting stuck.
3. **WaitAtPoint** waits the route's wait time, then **AdvancePatrol** picks the next point (loop or ping-pong) and the
   cycle repeats.
4. A guard without a valid route goes to **NoRoute** and idles forever.

### Combat and Alerted

- **Alerted**: Set Focus turns the guard toward the suspect (smoothly, at the controller's yaw rate). Entering the
  state cancels the patrol move, so the guard brakes and hovers while it watches you.
- **Combat**: the guard keeps facing you and chases your last known location. Swim To with **Follow Target** never
  finishes: it re-plans when you move more than 200 cm and holds position within 150 cm. Combat behaviour beyond the
  chase (attacks) is not implemented.

## Search flow

When the guard loses a suspicious player (meter ≥ 0.3) or the 5 s Combat grace time ends, its status becomes
**Searching** and the Searching branch runs:

1. **Begin Search** (on the Searching state itself) starts a new search memory around the last known location
   (`Origin`) and forgets previously checked points. It runs for the whole state and never completes it.
2. **GoToLKL** — swim to the last known location (Swim To, radius 100).
3. **LookAround** — **Look Around** stops and glances right, left, … and finally back ahead (Duration 4 s, Half Angle
   75°, Glance Count 3; turning at the controller's yaw rate).
4. **FindSweepPoint** — **Find Search Point** asks its **Source** for a place to check and writes it into
   `Searching.SearchPoint`. With the default source **Swim Nav Graph Nodes** that's a random nav-graph node that is
   - within **Radius** (1500 cm) of the search origin,
   - at least **Min Distance From Origin** (200 cm) from it (the guard already looked there),
   - not within **Visited Tolerance** (150 cm) of a point already checked,
   - reachable from the guard (same graph component).

   It **fails** once **Max Sweeps** (bound to `SearchSweepCount` = 3) points were checked, or when no candidate is
   left.
5. **GoToSweepPoint** — swim to that point, then **LookAround** again. Steps 3–5 repeat for each sweep point.
6. **GiveUp** — **Give Up Search** calls `ForgetLostTargets()` on the detection component: every player the guard
   can't see right now is forgotten, so the status drops to **Idle** immediately. On the next tick Searching's
   `OnTick` transition goes to Root, Root picks **Patrol**, and **ResumePatrol** sends the guard to the closest patrol
   point. (Give Up Search succeeds on its next tick; either its On Completed → Patrol or Searching's OnTick → Root fires — both end in Patrol.)

Other ways out of Searching:

- **You're seen again**: the re-sight boost lifts the meter to the threshold, status becomes Alerted (or Combat if it
  fills) and the guard leaves the search. Losing you again starts a fresh search.
- **45 s cap**: when the search clock (Max Search Duration) runs out and the meter has drained below the threshold, the
  status drops to Idle and the guard resumes patrol even mid-sweep.

### Extending the search

`FAtlantisSearchPointSource` is a seam: write a new `USTRUCT` deriving from it (override `FindPoint(Query, OutPoint)`)
and it shows up in Find Search Point's **Source** dropdown — e.g. checking hiding spots near the last known location.
Searching hiding spots is not implemented.

## Debugging

- **State Tree debugger** (State Tree editor → Debugger, works in PIE): shows the active state per guard.
- `Atlantis.AI.NavGraph.Draw 1` — each guard's current path (2 = nodes, 3 = links near the camera); `Atlantis.AI.Detection.Draw 1` — last known locations, status/meter text
  and search points.
- `UAtlantisPatrolComponent` → **Cursor** on the guard instance (Details panel while simulating) shows the current
  point and direction.
- Tests: `Atlantis.AI.Patrol.Cursor` covers loop wrap, ping-pong, 1/2-point routes and empty routes.

## Known limits

- Swimming guards only: Swim To fails for walking enemies, so their patrol falls through to RetryDelay.
- Walking enemies share ST_Enemy: in Combat their Swim To (even with Follow Target) fails immediately, so the tree re-selects from Root every tick. Not stuck, but they won't chase until land pathing exists (smoke-test in L_PerceptionTest).
- Patrol points and search points must be reachable through the nav graph.
- Wait time is per route in `BP_PatrolPath` (the interface supports per-point waits; the Blueprint returns the same
  value for every point).
