# Swimming AI Navigation

> Unreal has no 3D navigation, so swimming enemies use our own waypoint graph. You place nodes in the water, the
> game links them automatically, and the **Swim To** State Tree task finds a path through them and hands it to the
> engine's path following.
>
> Related docs: [Guard Detection.md](./Guard%20Detection.md), [Guard Patrol & Search.md](./Guard%20Patrol%20%26%20Search.md).

## Overview

```
Swim To (State Tree task)
  └─ UAtlantisNavGraphSubsystem::FindPath        (one per world, owns the graph)
       ├─ FAtlantisNavGraph                       (plain C++ graph + A*, unit tested)
       └─ IAtlantisNavNodeSource                  (where nodes come from)
            └─ AAtlantisNavNode                   (hand-placed waypoint actor)
  └─ AAIController::RequestMove(path)
       └─ UAtlantisPathFollowingComponent         (3D reach tests, per-segment traversal mode)
            └─ UAtlantisMovementComponent         (the player's movement component, swimming)
```

Code lives in `Source/ProjectAtlantis/{Public,Private}/AI/Navigation/` (files starting with `AtlantisNav…`,
`AtlantisPathFollowingComponent`) and `AI/StateTree/AtlantisStateTreeSwimToTask`.

## The waypoint graph

### Placing nodes

1. In the **Place Actors** panel search for **Atlantis Nav Node** (`AAtlantisNavNode`) and drag it into the level.
2. The node's location is where a swimming enemy's **capsule centre** will pass, so put it in open water, clear of
   geometry by at least the enemy's capsule size.
3. Every node must be **inside water**: a **Physics Volume** with **Water Volume** ticked. An enemy that leaves the
   water stops swimming, and Swim To only works while swimming.
4. Keep nodes **at least one capsule height below the water surface** (the top of the volume). Near the surface the
   swim movement damps upward motion and enemies get stuck bobbing under it.
5. Cover the space enemies need: patrol points, around obstacles, and the areas where they should search. Nodes
   further apart than **Max Link Distance** (1200 cm by default) never connect, so leave no gaps larger than that.
6. Nodes are invisible in game and have no collision; in the editor you see a billboard sprite.

The node has one property, **Traversal Modes** (bitmask, default **Swimming**). Leave it as it is: only Swimming is
supported today (see [Seams](#seams-extending-the-system)).

Tip: put all nodes in one outliner folder (e.g. `NavGraph`) so they are easy to hide and select.

### How links are built

The graph is built when the world begins play, from every actor that implements `IAtlantisNavNodeSource`:

- Every pair of nodes closer than **Max Link Distance** that shares a traversal mode is a candidate link.
- A candidate becomes a link only if an upright capsule (**Agent Radius** × **Agent Half Height**) can be swept
  between the two nodes without hitting anything on the `Pawn` collision channel. Players and other enemies are
  ignored, and water volumes only overlap, so they never block a link.
- Links are symmetric (A→B means B→A) and cost their 3D length.
- Nodes are then grouped into connected **components** (islands). Nodes in different components can't reach each
  other; pathfinding between them fails immediately.

### Settings

**Project Settings → Game → Atlantis AI Navigation** (`UAtlantisNavGraphSettings`, saved to `DefaultGame.ini`):

| Setting | Default | What it does |
|---|---|---|
| Max Link Distance | 1200 cm | Nodes further apart are never linked. |
| Agent Radius | 40 cm | Radius of the capsule swept between nodes. |
| Agent Half Height | 90 cm | Half height of that capsule (enemies swim upright). |
| Max Nearest Candidates | 8 | How many nearest nodes are tested for a clear line when a path enters or leaves the graph. |

Match Agent Radius / Half Height to the biggest swimming enemy's capsule. Changes apply the next time the graph is
built (next PIE).

### Checking the graph

**In game:** open the console and run

```
Atlantis.AI.NavGraph.Draw 1
```

Each node is drawn as a small sphere, links as lines. Colours are per component (cyan, green, yellow, magenta, orange,
turquoise, then repeat); a **red** node has no links at all. `Atlantis.AI.NavGraph.Draw 0` turns it off. If you see
more than one colour, part of your graph is cut off from the rest.

**In the editor (no PIE):** `UAtlantisNavGraphLibrary::BuildAndDescribeNavGraph(WorldContext)` is a Blueprint/Python
callable that rebuilds the graph of the given world and returns a one-line report:

```
Nodes: 48 | Links: 668 | Components: 1 | Isolated: 0 [] | Inside geometry: 0 []
```

`Isolated` lists nodes with no links; `Inside geometry` lists nodes whose capsule overlaps blocking geometry (move
them). From Python:

```python
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print(unreal.AtlantisNavGraphLibrary.build_and_describe_nav_graph(world))
```

The goal is **Components: 1, Isolated: 0, Inside geometry: 0**.

## Pathfinding

`UAtlantisNavGraphSubsystem::FindPath(From, To, Filter, bAllowDirectLeg, OutPath)` works on capsule-centre
locations and returns one of `EAtlantisNavPathResult`:

| Result | Meaning |
|---|---|
| Direct | The straight line From → To is clear (and direct legs are allowed); the graph isn't used. |
| Graph | Path found: From → nearest visible node → … A\* … → node nearest the goal → To. |
| NoStartNode | None of the nearest nodes has a clear line from the start. |
| NoEndNode | None of the nearest nodes has a clear line to the goal. |
| Unreachable | Start and end nodes are in different components (or A\* failed). |

"Visible node" means: of the **Max Nearest Candidates** nodes closest to the point, the first one the capsule sweep
can reach in a straight line.

Other queries: `GetReachableNodeLocations` (nodes within a radius that are in the same component as the searcher, used
by the search), `FindNearestVisibleNode`, `IsSegmentClear`, `Rebuild()` (BlueprintCallable) and `Describe()`.

## The Swim To task

**Swim To** (`FAtlantisStateTreeSwimToTask`, category Atlantis|AI) swims the controlled pawn to a 3D location.

| Property | Category | Default | Notes |
|---|---|---|---|
| AI Controller | Context | — | Bound automatically to the tree's AIController context. |
| Target Location | Input | (0,0,0) | Where the capsule centre should end up. Bind it (patrol point, last known location…). |
| Acceptance Radius | Parameter | 75 cm | Succeeds once this close, in 3D. |
| Allow Direct Leg | Parameter | true | Swim straight when the line is clear, skipping the graph. |
| Repath Distance | Parameter | 0 cm | Re-plan when the target has moved this far from where the current path leads. 0 = plan once on enter. |
| Follow Target | Parameter | false | Keep following: never succeeds, holds position on arrival and swims again when the target moves away. Failed paths are retried instead of failing the task. For chases. |

Behaviour:

- **Fails** if the pawn isn't swimming (logged once: "…is not swimming; Swim To only supports swimming enemies for
  now."), if there's no pawn / path following component, or if no path exists.
- **Succeeds** immediately if already within Acceptance Radius; otherwise **Running** until the move finishes
  (Succeeded on arrival, Failed if the move is blocked or aborted).
- **Repath Distance**: while moving, if the bound target moved further than this from the planned target, a new path
  is requested. Re-plans are rate limited to one every 0.25 s.
- **Follow Target**: the task stays Running for as long as its state is active. When the pawn has stopped and the
  target is more than Acceptance Radius + 50 cm away, it swims again. Combine it with Repath Distance for a chase
  (ST_Enemy's Combat state uses Acceptance Radius 150, Repath Distance 200, Follow Target on).
- **Leaving the state** stops the move with zero velocity, so the enemy stops and hovers (that's how Alerted guards
  halt mid-patrol).

## Path following

`UAtlantisPathFollowingComponent` replaces the engine's path following on `AAtlantisAIController` (and so on every
enemy controller). Swim To builds an engine `FNavigationPath` with no navigation data, shifted down to feet height
(the engine measures from the feet), with each segment's traversal mode stored in the path point flags.

- **Navmesh paths** (with navigation data) use the engine behaviour untouched — land enemies are not affected.
- **Swim graph paths** use 3D sphere reach tests instead of the engine's flat cylinder:
  - an intermediate point counts as reached within **Intermediate Point Tolerance** (30 cm, EditDefaultsOnly on the
    component) or once the pawn has moved past it;
  - the destination counts as reached within max(Acceptance Radius, Intermediate Point Tolerance), in 3D;
  - following always starts at path point 0 (the pawn's current location).
- Each segment is handled by its traversal mode. **Swimming** uses the engine's steering (3D; the movement component
  keeps vertical velocity while swimming). Any other mode logs a warning once and aborts the move.

Facing is done by the controller, not the movement component: `AAtlantisEnemyController` turns the pawn toward its
focal point at **Max Yaw Rate Degrees Per Second** (180 by default) and keeps pitch level. Enemies need
**Use Controller Rotation Yaw** on and **Use Acceleration For Paths** off (otherwise the movement component steers
toward the velocity, including pitch, and fights the controller).

## Seams (extending the system)

- **Node sources** (`IAtlantisNavNodeSource::GatherNavNodes`): anything in the world implementing this C++ interface
  contributes nodes. `AAtlantisNavNode` is the only source today; an auto-grid volume can be added later as another
  source without touching pathfinding. (C++ only: the interface can't be implemented in Blueprint.)
- **Traversal modes** (`EAtlantisTraversalMode`: None, Terrestrial, Swimming, SurfaceWalking): every node has a mode
  mask, every link and path segment a mode, and queries a `FAtlantisNavQueryFilter` with allowed modes. Only Swimming
  is used. A surface-walking handler would be a new case in `UAtlantisPathFollowingComponent::FollowPathSegment`.

## Known limits

- **Static graph.** Built once at begin play (lazily on the first query when needed). Moving nodes or obstacles at
  runtime isn't picked up; call `Rebuild()` after doing so.
- **Swimming only.** Swim To fails for enemies that aren't swimming (land enemies have no path through it), and the
  graph only supports the Swimming mode.
- **Path entry/exit.** The graph is entered at the nearest node with a clear line, which may be slightly behind the
  enemy; a sparse graph gives zig-zaggy paths.
- **AI movement default drift.** `AAtlantisAICharacter` now uses `UAtlantisMovementComponent` (the player's movement
  component). Every AI Blueprint that didn't override them picked up its defaults: **Braking Deceleration Swimming
  0 → 250**, **Max Swim Acceleration 2000**, **Max Vertical Swim Speed 150**. This is intentional: same swim feel as the
  player, and the braking lets an Alerted guard stop and hover instead of drifting. `BP_PuzzleGuard` uses Buoyancy 1,
  Gravity Scale 0, Max Swim Speed 300.

## ST_Enemy: Scheduled Tick Policy must be Denied

In UE 5.7 a State Tree can use **scheduled ticks**: when no active task asks to tick, the tree sleeps. With only Swim
To moves and engine Delay tasks running, ST_Enemy ticked only a few times per second, so detection was ~30× too slow
(the old Perception Evaluator lived in the tree) and status transitions came late.

Detection now ticks on its own component, but the tree still has to notice status changes immediately (the `OnTick`
transitions and the evaluator outputs). So in ST_Enemy keep **Schema → Scheduled Tick Policy = Denied**. Don't change it
back to Default/Allowed.

## Tests

`Atlantis.AI.Nav.Graph` (`Private/Tests/AtlantisNavGraph.spec.cpp`) covers routing around a wall, separate components,
mode filtering, radius queries, link symmetry/limits and the engine path conversion. See
[Automated Tests.md](./Automated%20Tests.md) for how to run them.
