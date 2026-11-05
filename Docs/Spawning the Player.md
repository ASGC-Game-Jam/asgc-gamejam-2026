# Spawning the Player

The short version, and the part that surprises everyone: **you do not spawn the player.** The
GameMode does, automatically, before your code runs. Almost every "how do I spawn my character"
problem is really "my GameMode isn't wired up," and the fix is three settings rather than any
code at all.

Related: [Choosing a Gameplay Class.md](./Choosing%20a%20Gameplay%20Class.md) for what each class
is, [Game Framework Classes.md](./Game%20Framework%20Classes.md) for how we split them.

---

## What actually happens

When a player joins — including you pressing Play — the engine runs this without being asked:

```
Player joins
   │
   ▼
GameMode::PostLogin
   │
   ▼
GameMode::RestartPlayer(Controller)
   │
   ├─► FindPlayerStart(Controller)          ── picks an APlayerStart in the level
   │      └─ ChoosePlayerStart              ── prefers an unoccupied one
   │
   ▼
GameMode::RestartPlayerAtPlayerStart(Controller, StartSpot)
   │
   ├─► GetDefaultPawnClassForController     ── reads DefaultPawnClass
   ├─► SpawnDefaultPawnAtTransform          ── SpawnActor at the start's transform
   │
   ▼
Controller->Possess(Pawn)                   ── input now reaches the pawn
```

Two things follow from this. First, a pawn you place in the level by hand is *not* the player —
the GameMode spawns a separate one at the PlayerStart and possesses that. Second, possession is
what connects input; an unpossessed pawn is scenery no matter how much input logic it contains.

---

## The three things that must be true

**1. The level uses your GameMode.** World Settings ▸ GameMode Override, or the project-wide
default in Project Settings ▸ Maps & Modes. `Lvl_ThirdPerson` overrides to `BP_AtlantisGameMode`.

**2. The GameMode names your pawn.** `DefaultPawnClass` in the GameMode's class defaults.
`BP_AtlantisGameMode` sets it to `BP_AtlantisPlayerCharacter`.

**3. The level contains an `APlayerStart`.** Drag one in from Place Actors. `Lvl_ThirdPerson`
has one.

Get all three right and the player spawns. Miss any one and you get one of the failures below.

---

## Why "it didn't spawn" — the real causes

### The PlayerStart is blocked

This is the most common one, and the most confusing because nothing obviously breaks.

`APawn`'s constructor sets:

```cpp
SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
```

If the capsule doesn't fit at the PlayerStart, the engine tries to nudge it — and if it still
collides, **it does not spawn the pawn at all.** You get a warning in the Output Log and an
otherwise silent failure: no character, no input, camera sitting at the world origin.

A PlayerStart sunk slightly into the floor or clipping a wall is enough. The actor shows a
capsule preview and turns red in the viewport when it's blocked — check that first.

### There is no PlayerStart at all

`RestartPlayerAtPlayerStart` opens with:

```cpp
if (!StartSpot)
{
    UE_LOG(LogGameMode, Warning, TEXT("RestartPlayerAtPlayerStart: Player start not found"));
    return;
}
```

It returns. **No pawn is spawned** — it does not fall back to the origin. PIE is more forgiving
than a cooked build here, so "it worked in the editor" is not evidence the level is set up
correctly. Always place a real PlayerStart.

### The GameMode isn't the one you edited

Easy to lose an afternoon to. A level's GameMode Override beats the project default, so editing
the project-wide GameMode changes nothing if the level overrides it — and vice versa. Check
World Settings before assuming your change didn't apply.

### You spawned it yourself on a client

`SpawnActor` on a client creates an actor that exists only on that machine and is never
replicated. Pawns must be spawned by the **server**. Guard anything that spawns with
`HasAuthority()` and let replication deliver it to clients.

---

## Things people reach for, and when they're actually fine

**Dragging a Character Blueprint into the level.** Doesn't make it the player. The GameMode still
spawns its own at the PlayerStart, so you end up with two — one you control and one standing
where you left it.

**`Auto Possess Player` on a placed pawn.** Set it to `Player 0` and the placed pawn *is*
possessed. Genuinely useful for a quick single-player test or a prototype level. Don't ship it:
it doesn't work for multiplayer, it bypasses the GameMode entirely, and it breaks the moment you
need respawning.

**Calling `SpawnActor` yourself.** Right for NPCs, enemies, projectiles, and any second pawn. For
*the player's* pawn, prefer the GameMode path — you get PlayerStart selection, possession, and
respawn for free. If you truly need control, override a hook below rather than bypassing it.

---

## Where to hook in

Override these on your GameMode rather than replacing the flow:

| Hook | Use it to |
| --- | --- |
| `GetDefaultPawnClassForController` | Pick a different pawn per player — class selection, team, spectator |
| `ChoosePlayerStart` | Custom spawn selection — furthest from enemies, team-specific, round-based |
| `FindPlayerStart` | Full control over start resolution, including `PlayerStartTag` matching |
| `SpawnDefaultPawnAtTransform` | Change spawn parameters, e.g. collision handling or deferred spawn |
| `RestartPlayer` / `RestartPlayerAtTransform` | Trigger a (re)spawn yourself, at a start or an arbitrary transform |
| `PostLogin` | React to a player joining, before or after their first spawn |

`APlayerStart` also carries a `PlayerStartTag`, which `FindPlayerStart` matches against the
incoming player's name — the built-in way to route specific players to specific starts.

---

## Respawning

Death is not special: killing the pawn and calling `RestartPlayer(Controller)` again runs the
same path and gives the player a fresh pawn at a start point. The PlayerController and
PlayerState survive — which is exactly why score and team live there and not on the pawn. See
[Choosing a Gameplay Class.md](./Choosing%20a%20Gameplay%20Class.md).

A cheaper alternative for a jam game: don't destroy the pawn at all, just move it back. That is
what `AAtlantisPlayerCharacter::MoveToStart()` is for — it returns the character to the transform
captured at `BeginPlay`, keeping all its state. Good for respawn-on-fall-out-of-world; not a
substitute for real respawning if the pawn accumulates state that should reset.

> ⚠️ `MoveToStart` and `MoveToTransform` are not authority-guarded today. Called on a client they
> move the actor locally and desync it. Treat them as server-only until that is fixed.

---

## Multiplayer rules

- **Only the server spawns pawns.** Replication delivers them to clients.
- **Only the server runs the GameMode**, so all of the hooks above are server-side by definition.
- **`Possess` is server-only.** Clients get the result through the pawn's replicated owner.
- A client that needs to trigger a respawn sends a `Server*` RPC on its PlayerController; the
  GameMode does the work.

---

## Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| No character, camera at origin | No PlayerStart, or the PlayerStart is blocked |
| "Couldn't spawn Pawn of type … " in the log | Blocked spawn, or `DefaultPawnClass` is unset |
| Character spawns but input does nothing | Nothing possessed it, or input contexts were never added |
| Two characters, one frozen | A pawn was placed in the level *and* the GameMode spawned one |
| Works in PIE, broken in a build | Relying on PIE's forgiving fallback instead of a real PlayerStart |
| Works in single player, broken with two clients | Spawned on the client instead of the server |
| Everything spawns at the same spot | One PlayerStart; add more, or override `ChoosePlayerStart` |

Check the Output Log first. `LogGameMode` warns on every one of these, and the message names the
pawn class and the transform it tried.

---

## Supporting Unreal documentation

- [Player Start Actor](https://dev.epicgames.com/documentation/unreal-engine/player-start-actor-in-unreal-engine?lang=en-US) —
  what a PlayerStart is and how to place one
- [AGameModeBase::RestartPlayer](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/AGameModeBase/RestartPlayer?application_version=5.1) —
  *"tries to spawn the player's pawn at the location returned by FindPlayerStart"*
- [SpawnDefaultPawnFor](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/AGameModeBase/SpawnDefaultPawnFor) ·
  [SpawnDefaultPawnAtTransform](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/AGameModeBase/SpawnDefaultPawnAtTransform) —
  the two spawn entry points, called during `RestartPlayer`
- [Game Mode and Game State](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine) —
  why the GameMode is server-only, and what that means for spawning
- [Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine) —
  Pawns, possession, and how controllers relate to them

> The collision-handling default and the "no start spot, no pawn" behaviour were read from the
> 5.7 engine source (`APawn::APawn`, `AGameModeBase::RestartPlayerAtPlayerStart`), since the
> published pages do not state either.
