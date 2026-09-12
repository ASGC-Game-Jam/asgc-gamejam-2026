# Game Framework Classes

How Project Atlantis structures its GameMode, PlayerController, PlayerState, and player
Character: a C++ base class per role, with a thin Blueprint leaf on top.

Read this before adding a gameplay class, and follow the same split.

Deciding *which* framework class a piece of state belongs on in the first place —
GameMode vs GameState vs PlayerController vs PlayerState vs Pawn — is a separate question,
covered in [Choosing a Gameplay Class.md](./Choosing%20a%20Gameplay%20Class.md).

---

## The shape

Every framework role is two classes, not one.

```
        C++ base                      Blueprint leaf              lives in
  ──────────────────────────    ────────────────────────    ──────────────────────
  AAtlantisPlayerCharacter   ◄─ BP_AtlantisPlayerCharacter   Core/Characters
  AAtlantisPlayerState       ◄─ BP_AtlantisPlayerState       Core/PlayerState
  AAtlantisPlayerController  ◄─ BP_AtlantisPlayerController   Core/PlayerControllers
  (AGameModeBase)            ◄─ BP_AtlantisGameMode          Core/GameModes
```

They are wired together through the GameMode's class defaults, never by hard references
between each other:

```
Lvl_ThirdPerson  ──(GameMode override)──►  BP_AtlantisGameMode
                                                  │
                                    DefaultPawnClass      ──►  BP_AtlantisPlayerCharacter
                                    PlayerControllerClass ──►  BP_AtlantisPlayerController
                                    PlayerStateClass      ──►  BP_AtlantisPlayerState
```

**C++ owns contracts and state. Blueprint owns composition and content.** That is the whole
rule; everything below is a consequence of it.

| Goes in the C++ base | Goes in the Blueprint leaf |
| --- | --- |
| Replicated properties and RepNotifies | Component layout (mesh, camera boom, collision) |
| Change delegates other systems bind to | Asset references (skeletal mesh, anim BP, materials) |
| Interfaces the class implements | Input Mapping Context assignment |
| `virtual` hooks designers override | Tuning values a designer should own |
| Anything that needs code review | Anything a designer should change without a recompile |

---

## Why it is shaped this way

**Designers should never edit a base class.** `Tech Standards.md` 2.5 is explicit: `Core` is a
"don't touch these" folder, and designers make tweaks in child classes that expose
functionality. The C++/Blueprint split makes that structural rather than a rule people
remember — the base is a source file behind a compile step, and the leaf is where the Content
Browser points.

**This is Epic's own recommendation, not a house invention.** Their guidance is that [the best
approach to combine Blueprint and C++ is to use C++ as the foundation and build Blueprint
classes on top of it](https://dev.epicgames.com/documentation/en-us/unreal-engine/coding-in-unreal-engine-blueprint-vs-cplusplus),
because exposing precisely what you want from C++ gives more control and avoids
overly large, hard-to-follow Blueprints. The same page supplies our threshold for promoting
something to native: **if more than one or two Blueprints use it, it should probably be C++.**

**C++ diffs. `.uasset` does not.** Logic in C++ is reviewable in a pull request, mergeable when
two people touch it, and greppable. Logic in a Blueprint is a binary blob that produces merge
conflicts nobody can resolve. Every piece of behaviour we might need to review, debug from a
callstack, or merge belongs on the C++ side for that reason alone.

**Porting Blueprint logic to C++ finds bugs.** Moving the controls API off the Blueprint graph
surfaced two latent defects that had been invisible in the graph view: `ClearAllControls`
emptied its array *before* iterating it to withdraw contexts, so every context stayed applied
forever; and two of its three event dispatchers were declared but never broadcast from
anywhere. Neither is visible when reading a node graph, and both are obvious in twenty lines of
C++. Treat that as the standing argument for the migration, not an anecdote.

**Some things are simply better in each language.** Epic frames the distinction as
[programming versus scripting: C++ defines systems through instructions, Blueprint defines
behaviours by interfacing with existing
systems](https://dev.epicgames.com/documentation/en-us/unreal-engine/balancing-blueprint-and-cplusplus?application_version=4.27).
Replication with RepNotifies, interfaces, and anything performance-sensitive want C++.
Component hierarchies, asset wiring, and designer-facing tuning want Blueprint, where they are
visual and need no compile. Fighting that split costs you either merge pain or iteration speed.

**One class per framework role, because the engine already divides them that way.**
[GameMode, PlayerController, PlayerState and Pawn each carry a distinct
responsibility](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine) —
rules, input and possession, per-player data that outlives a pawn, and the physical avatar.
Collapsing them into one god class fights the framework and breaks the moment you need state to
survive a respawn.

**We reparented the template Blueprints rather than rebuilding them.**
`BP_AtlantisPlayerCharacter` carries roughly 1,700 nodes of working template setup — component
layout, input wiring, animation hookup — and `BP_AtlantisPlayerController` about 2,500.
Reparenting slides a C++ base underneath all of that while it keeps working, instead of
re-authoring it by hand. The Blueprint keeps its content; we gain a code seam.

> ⚠️ Reparenting can silently drop variables or break nodes when the new parent shadows a name
> the child already used. Always recompile and read the Message Log immediately after a
> reparent, before you commit.

**Template names describe a camera, not a game role.** `BP_ThirdPersonCharacter` and
`BP_ThirdPersonPlayerController` are Unreal template artifacts; "third person" is a perspective,
not a thing in Atlantis. They were replaced by `BP_Atlantis*` names that say what the class is
for.

**No project infix on the class name.** An earlier pass used `BP_PAPlayerCharacter`. That was
dropped: `Tech Standards.md` 2.2 already namespaces everything under the
`Content/ProjectAtlantis` top-level folder, so a `PA` in the asset name is redundant path length
for no information.

**`BP_AtlantisGameMode` is deliberately empty.** It has no graph, only class defaults. GameMode
wiring is *data* — which pawn, which controller, which player state — and data does not want an
event graph. If the GameMode ever needs behaviour, that behaviour goes into a C++
`AAtlantisGameMode` base and this asset stays a config shell.

---

## How to interface with it

**Adding designer-facing content** — mesh, camera, input context, tuning: open the `BP_Atlantis*`
leaf. Never the C++ base.

**Adding behaviour or state** — open the C++ base. If designers need to call it, mark it
`UFUNCTION(BlueprintCallable)`; if they need to override it, `BlueprintNativeEvent`. A method
with no `UFUNCTION` is invisible to the leaf, which defeats the point of the split. A protected
`BlueprintCallable` is callable from derived Blueprints but not from outside code, which is the
right visibility for something only the leaf should drive.

**Changing which classes a level uses:** set them in `BP_AtlantisGameMode`'s class defaults, then
confirm the level's World Settings GameMode override points at it. Do not hard-reference one
framework class from another.

### Player state

Bind to the delegates on `AAtlantisPlayerState` rather than polling it each tick. See
[Replicated State Pattern.md](./Replicated%20State%20Pattern.md).

### Controls

`AAtlantisPlayerController` owns the player's Enhanced Input mapping contexts, and separates two
ideas that are easy to conflate:

- `CurrentMappingContexts` — the **desired** set of controls
- `bControlsEnabled` — whether that set is currently pushed to the input subsystem

Keeping them apart is what lets gameplay add and remove controls while input is suppressed (a
cutscene, a menu) and have the correct set restored when control returns, without every caller
having to remember what was active.

This mirrors how Enhanced Input is meant to be driven: [contexts are added and removed at
runtime through the Enhanced Input Local Player Subsystem, and prioritised to resolve collisions
between actions competing for the same
input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine).
The controller is simply the one place that owns *which* contexts are live.

```cpp
Controller->AddControls(SwimmingContext);     // adds to the set; applies now if enabled
Controller->RemoveControls(SwimmingContext);  // drops from the set; withdraws now if enabled

Controller->DisableAllControls();             // withdraw everything, remember the set
Controller->EnableAllControls();              // re-apply the remembered set

Controller->ClearAllControls();               // withdraw everything and forget the set
```

React to changes through `OnControlsChanged`, `OnControlsEnabled`, and `OnControlsDisabled`
rather than re-reading the array each frame. All three are `BlueprintAssignable`.

Every context is applied at priority `0`. Supporting per-context priority means
`CurrentMappingContexts` becomes an array of structs, because `EstablishMappingContexts`
re-applies the whole set at one priority.

### Repositioning the player character

```cpp
Character->MoveToStart();                  // back to the transform captured at BeginPlay
Character->MoveToTransform(TargetXform);   // to an arbitrary transform
```

Both move the actor locally and are not authority-guarded, so treat them as server or
single-player only until they are. See Known gaps.

---

## Repeat this pattern

For any new gameplay class that designers will touch — a weapon, an interactable, a vehicle:

1. **Create the C++ base** in `Source/ProjectAtlantis/Public/Core/` and `Private/Core/`, with the
   correct type prefix (`A` for actors, `U` for objects) and `PROJECTATLANTIS_API`.
2. **Put the contract there**: replicated state, change delegates, interfaces, and the `virtual`
   or `BlueprintNativeEvent` hooks the leaf is meant to override.
3. **Expose what designers need** with `UFUNCTION(BlueprintCallable)` / `BlueprintPure`, and
   `UPROPERTY(BlueprintReadOnly)` or `EditDefaultsOnly` as appropriate.
4. **Create the Blueprint leaf** as `BP_Atlantis*` (or a descriptive name) in the matching
   `Content/ProjectAtlantis/Core/...` folder, parented to the C++ class.
5. **Put composition there**: components, asset references, designer tuning.
6. **Wire it** through class defaults or a data asset, never a hard cross-reference.
7. **Recompile, check the Message Log, and confirm zero warnings** per `Tech Standards.md` 3.1.

Base classes that are fundamental to the project live in `Core`; the designer-facing variants
built on them belong in `Placeables` or another content folder, per `Tech Standards.md` 2.5.

### Migrating existing Blueprint logic into a C++ base

Reparenting alone is not enough. Before you reparent, **delete the Blueprint's own copies of any
function, dispatcher, or variable the C++ base now declares** — they collide with the inherited
members and the reparent will error. Then recompile and read the Message Log.

---

## Known gaps in the current implementation

Empty this list as items land — it is a snapshot, not part of the convention.

- **`BP_AtlantisPlayerController` has not been reparented** to `AAtlantisPlayerController`, and
  still holds the Blueprint versions of the controls functions, dispatchers, and variables. They
  must be deleted before reparenting.
- **The rest of that controller's `BeginPlay` graph is still Blueprint** — touch-control
  detection and the touch widget spawn were not ported.
- **`MoveToStart` and `MoveToTransform` are not authority-guarded or replicated.** Called on a
  client, they teleport the actor locally and desync it.
- **`StartTransform` is captured before `Super::BeginPlay()`** in
  `AAtlantisPlayerCharacter::BeginPlay`. It works, but calling `Super` first is the convention.
- **`AAtlantisPlayerState` properties are `protected`,** so a C++ subclass can write them
  directly and bypass the authority-guarded setters. Deliberate, but worth knowing.
- **Six orphaned graph exports** are still tracked for Blueprints that were renamed, moved, or
  deleted — among them `BP_PAPlayerCharacter`, `BP_ThirdPersonCharacter`, and the old
  `Core/Characters/BP_AtlantisPlayerState`. The Keystone plugin now prunes these itself: they
  clear automatically the first time the editor starts with the rebuilt plugin, with no menu
  action needed.
- **`BP_ThirdPersonGameMode` is a redirector** left behind when it was renamed to
  `BP_AtlantisGameMode`. Remove it with **Fix Up Redirectors** on `Core/GameModes` rather than
  deleting it, so anything still pointing at the old name is repointed first.
- **The Keystone plugin's latest changes are not built yet** — the automatic orphan cleanup, the
  startup sweep, and the shared Keystone menu. The game module is built; its only changes since
  the last build are comments and whitespace. Rebuild with the editor closed.

---

## Supporting Unreal documentation

The C++ base / Blueprint leaf split is Epic's own recommendation, not a house invention:

- [Coding in Unreal Engine: Blueprint vs. C++](https://dev.epicgames.com/documentation/en-us/unreal-engine/coding-in-unreal-engine-blueprint-vs-cplusplus) —
  *"the best approach to combine Blueprint and C++ is to use C++ as the foundation and build
  Blueprint classes on top of it."* Also the source of the rule that anything used by more than
  one or two Blueprints should be native C++.
- [Balancing Blueprint and C++](https://dev.epicgames.com/documentation/en-us/unreal-engine/balancing-blueprint-and-cplusplus?application_version=4.27) —
  the longer argument, including why exposing precisely what you want from C++ keeps Blueprints
  from becoming large and hard to follow.
- [CPP and Blueprints Example](https://dev.epicgames.com/documentation/en-us/unreal-engine/cpp-and-blueprints-example) —
  a worked example of the same split.
- [Gameplay Framework in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine) —
  what GameMode, PlayerController, PlayerState, and Pawn are each responsible for, which is why
  they are separate classes here rather than one god object.

For the controls API:

- [Enhanced Input in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine) —
  Input Actions, Mapping Contexts, Modifiers, and Triggers. Confirms the design we rely on:
  contexts are added and removed at runtime through the Enhanced Input Local Player Subsystem,
  and prioritised to resolve collisions between actions competing for the same input.
- [UInputMappingContext](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/EnhancedInput/UInputMappingContext) —
  API reference for the context objects `CurrentMappingContexts` holds.
- [Add Mapping Context](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Input/AddMappingContext) —
  the Blueprint node the C++ call replaced, including its priority and options parameters.
- [Input Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/input-overview-in-unreal-engine) —
  where input sits in the wider framework.

For the change-event fan-out, see the reference list in
[Replicated State Pattern.md](./Replicated%20State%20Pattern.md).
