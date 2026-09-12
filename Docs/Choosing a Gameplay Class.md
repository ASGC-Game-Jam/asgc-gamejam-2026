# Choosing a Gameplay Class

Unreal splits "the game" across half a dozen classes with confusingly similar names. Putting
something in the wrong one is the single most common structural mistake people make starting
out, and it usually surfaces late — as state that vanishes on respawn, or a value one player can
see and another cannot.

This is the decision guide. For how we split each of these between C++ and Blueprint, see
[Game Framework Classes.md](./Game%20Framework%20Classes.md).

---

## The whole decision, in two questions

**1. How long must it outlive?**

```
Pawn / Character   ──►  destroyed on death, respawn, or unpossess
PlayerController   ──►  as long as that client stays connected
PlayerState        ──►  as long as that client stays connected
GameState          ──►  one level
GameMode           ──►  one level, server only
GameInstance       ──►  the whole application run, across every level load
```

**2. Who has to be able to see it?**

```
just the server            ──►  GameMode, GameSession
just this player's machine ──►  PlayerController, LocalPlayer, HUD
every connected player     ──►  PlayerState (per-person), GameState (per-match)
nobody but this machine    ──►  GameInstance
```

Answer both and the class picks itself. Almost every mistake below is answering only the second
one and forgetting the first.

---

## The reference table

| Class | Lives for | Exists on | Replicated | Use it for |
| --- | --- | --- | --- | --- |
| `UGameInstance` | the whole app run | every machine, independently | ❌ never | Save data, settings, anything that must cross a level load |
| `AGameModeBase` | one level | **server only** | ❌ never | Rules, spawn logic, win conditions, login handling |
| `AGameStateBase` | one level | server + all clients | ✅ fully | Match state everyone sees: score, timer, phase, player list |
| `APlayerController` | while connected | server + **owning client only** | ✅ to its owner | Input, camera, UI ownership, client→server RPCs |
| `APlayerState` | while connected, **survives respawn** | server + all clients | ✅ fully | Per-player data others must see: name, score, team |
| `APawn` / `ACharacter` | until death or unpossess | server + all clients | ✅ | The physical body: movement, collision, mesh |
| `AGameSession` | one level | **server only** | ❌ | Login approval, online session wrapper |
| `ULocalPlayer` | across maps | client only — **zero on a dedicated server** | ❌ | Splitscreen slots, local-player subsystems |

---

## The classes, and what each is actually for

### `UGameInstance` — the only thing that survives a level load

[A high-level manager object for an instance of the running game, spawned at game creation and
not destroyed until the game instance shuts
down](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameInstance?lang=en-US).
One per standalone game; one per PIE instance when testing in-editor.

**Use it for:** anything that must outlive a level. Save-game payloads, audio settings, the
player's progression, a "which mission did we just finish" flag.

**Don't use it for:** anything replicated. It is per-machine and has no networking. The server's
GameInstance and a client's GameInstance are unrelated objects.

> Prefer a `UGameInstanceSubsystem` over adding members to a GameInstance subclass. Subsystems
> [share the lifetime of the game instance](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine)
> with automatic `Initialize` / `Deinitialize`, and keep unrelated systems from piling into one
> class. We already do this — see `UScenePerformanceAnalyticsSubsystem`.

### `AGameModeBase` — the rules, and only the server has them

[The Game Mode is not replicated to any remote clients that join in a multiplayer game; it
exists only on the
server](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine).

**Use it for:** which pawn to spawn, where to spawn it, when the match ends, who is allowed to
join, what happens on death.

**Don't use it for:** anything a client needs to read. `GetGameMode()` returns null on a client,
and that is not a bug. Put the client-visible half in GameState.

> `AGameModeBase` is the trimmed base. `AGameMode` adds the match state machine
> (`WaitingToStart` / `InProgress` / `WaitingPostMatch`). We are on `GameModeBase`, so if you
> want match phases you either move up to `AGameMode` or model them yourself on GameState.

### `AGameStateBase` — the match, as everyone sees it

[Spawned by GameModeBase, exists on both the client and the server, and is fully
replicated](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine).
It holds what is *"specific to the Game Mode but not specific to any individual player"* —
team scores, elapsed time, which objectives are complete, the list of connected players.

**Use it for:** the scoreboard's shared half, match timers, world-level flags.

**Don't use it for:** anything about one person. That is PlayerState.

### `APlayerController` — one player's input and view

[One exists on each client per player on that machine. They are replicated between the server
and the associated client, but **not** to other clients](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine).
The server has one for every player; a client has only its own.

**Use it for:** input handling and mapping contexts, camera control, opening and owning UI,
and as the place client→server RPCs live.

**Don't use it for:** anything another player must see. You will test in single player, it will
work, and it will break the moment a second client connects. This is the most common version of
the mistake.

### `APlayerState` — one player, visible to everyone

[Created for every player on a server (or in a standalone game), replicated to all clients, and
contains network-relevant information about the player such as name and
score](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APlayerState).

The critical property: PlayerControllers and PlayerStates
[are not destroyed and respawned like Pawns often are](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine).
State here survives death.

**Use it for:** score, kills, team, display name, loadout — anything per-person that another
player's HUD needs, or that must outlive the body.

**Don't use it for:** anything that should reset on respawn. That belongs on the Pawn.

See [Replicated State Pattern.md](./Replicated%20State%20Pattern.md) for how we broadcast changes
from here.

### `APawn` / `ACharacter` — the body, and only the body

[Pawns are the physical representations of players and creatures in a
level](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine);
Characters are Pawns with a mesh, collision, and `UCharacterMovementComponent` for a
vertically-oriented humanoid that walks, jumps, flies, and swims.

**Use it for:** movement, collision, animation, physical interaction, and state that *should*
reset when you die — current stamina, current ammo in the gun you are holding.

**Don't use it for:** anything that must survive respawn. This is the mistake that produces
"my score resets every time I die."

### `AGameSession` — server-side session plumbing

[A game-specific wrapper around the session interface, created and owned by the game mode, which
only exists on the server when running an online
game](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/AGameSession).
Handles login approval and arbitration.

**Use it for:** rejecting a join, hooking `PostLogin` / `NotifyLogout`, wrapping online session
calls.

**Don't use it for:** gameplay. It is join-time plumbing.

### `ULocalPlayer` — the seat, not the person

[Each player active on the current client or listen server has a LocalPlayer. It stays active
across maps, and there may be several in splitscreen or couch co-op. **There will be zero on
dedicated servers.**](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ULocalPlayer)

**Use it for:** reaching local-player subsystems. This is why `AAtlantisPlayerController` gets
Enhanced Input via `ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())`
— input mapping contexts belong to a seat at the machine, not to a player identity, which is
also why that call correctly returns null on a remote controller.

---

## What happens when you change levels

Lifetime is the axis people get wrong, and a level change is where it bites. Unreal has
[two ways to travel — seamless and non-seamless — the main difference being that seamless travel
is non-blocking](https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine),
and they treat these classes very differently.

| Class | Non-seamless travel | Seamless travel |
| --- | --- | --- |
| `UGameInstance` | ✅ survives | ✅ survives |
| `ULocalPlayer` | ✅ survives | ✅ survives |
| `AGameModeBase` | ❌ destroyed, new one spawned | ❌ carried to the transition map only, then replaced |
| `AGameStateBase` | ❌ destroyed, new one spawned | ❌ carried to the transition map only, then replaced |
| `AGameSession` | ❌ destroyed | ❌ carried to the transition map only, then replaced |
| `APlayerController` | ❌ destroyed, client reconnects | ❌ new one created; copies only what you copy |
| `APlayerState` | ❌ destroyed | ⚠️ **travels, then is copied into a new one** — see below |
| `APawn` / `ACharacter` | ❌ destroyed | ❌ destroyed, respawned by the new GameMode |
| `AHUD`, `APlayerCameraManager` | ❌ destroyed | ✅ carried over by the PlayerController |

**Non-seamless travel is a full teardown.** Every actor in the world is destroyed and clients
disconnect and reconnect. Only `UGameInstance` and `ULocalPlayer` survive, and they survive
because they were never part of the world in the first place. If you need something to cross a
non-seamless transition, GameInstance is the only answer.

**Seamless travel is more subtle than "things persist."** `AGameModeBase::GetSeamlessTravelActorList`
is explicit about what carries: PlayerStates are appended *always*, while the GameMode, GameState
and GameSession are added **only when travelling to the transition map**. They are not carried on
to the destination — new ones are spawned there. So GameState does *not* ferry data between
levels, even seamlessly.

### The PlayerState handoff, and the trap in it

At the destination a new PlayerController is created and `SeamlessTravelFrom(OldPC)` runs, which
does three things in this order:

```cpp
OldPC->PlayerState->Reset();                        // 1
OldPC->PlayerState->SeamlessTravelTo(PlayerState);  // 2  -> DispatchCopyProperties -> CopyProperties
OldPC->PlayerState->Destroy();                      // 3
```

`APlayerState::Reset()` calls `SetScore(0)`. It runs **before** the copy. So the engine's built-in
`Score` is zeroed on its way across and arrives as `0`, no matter what it was — a genuine
surprise, and the reason "my score vanished after the level change" is a recurring bug report.
If `Score` must survive, either override `Reset()` or keep the value in your own property
instead.

Your own properties are safe by default: `Reset()` only touches the engine's fields, so anything
you copy in your `CopyProperties` override arrives intact. That is exactly what
`AAtlantisPlayerState::CopyProperties` does — see
[Replicated State Pattern.md](./Replicated%20State%20Pattern.md) for why it assigns directly and
fires no notifies.

### Practical rule

Ask *"must this survive a level change?"* before *"who needs to see it?"*, because the answer
eliminates most of the table:

- **Yes, always** → `UGameInstance` or a `UGameInstanceSubsystem`. Nothing else is guaranteed.
- **Yes, but only across seamless travel** → `APlayerState`, in your own property, copied in
  `CopyProperties`.
- **No** → everything else is fair game.

---

## Where this actually goes wrong

| Symptom | What happened | Where it belongs |
| --- | --- | --- |
| "My score resets when I die" | It was on the Pawn, which is destroyed on respawn | `APlayerState` |
| "Other players can't see my score" | It was on the PlayerController, which isn't replicated to other clients | `APlayerState` |
| "GameMode is null on the client" | Working as designed — GameMode is server-only | Read `AGameStateBase` instead |
| "My data vanished on level change" | PlayerState and GameState do not survive a level load on their own | `UGameInstance`, or seamless travel + `CopyProperties` |
| "Score is zero after seamless travel" | `APlayerState::Reset()` zeroes `Score` before the copy runs | Your own property, or override `Reset()` |
| "GameState data didn't carry to the next level" | GameState is only kept as far as the transition map, then replaced | `UGameInstance` |
| "The match timer disagrees between players" | Each client computed it locally | Replicate it on `AGameStateBase` |
| "Input breaks on the dedicated server" | Reached for `ULocalPlayer`, which does not exist there | Guard on the null subsystem |
| "Everyone got someone else's HUD popup" | Multicast RPC where a client RPC was wanted | `APlayerController`, `Client*` RPC |

---

## What about "SessionState"?

There is no `ASessionState` class in Unreal. The name usually means one of three different
things:

- **`AGameSession`** — the server-side login and arbitration wrapper described above.
- **The Online Subsystem
  [Session Interface](https://dev.epicgames.com/documentation/unreal-engine/online-subsystem-session-interface-in-unreal-engine)** —
  the actual create / find / join / destroy of an online session, and `FOnlineSessionSettings`.
  This is what you want for matchmaking and invites.
- **`UGameInstance`** — where people usually *mean* to put "data that lives for the whole
  session," because it is the only thing that survives a level load.

If someone says "put it in session state," ask which of those three they mean. Nine times out of
ten the answer is a `UGameInstanceSubsystem`.

---

## Quick decision guide

```
Does it need to survive a level load?
├─ yes ─► UGameInstance (prefer a UGameInstanceSubsystem)
└─ no
   │
   Is it a rule or decision only the server makes?
   ├─ yes ─► AGameModeBase
   └─ no
      │
      Is it about the match rather than a person?
      ├─ yes ─► AGameStateBase
      └─ no
         │
         Do other players need to see it?
         ├─ yes ─► APlayerState
         └─ no
            │
            Should it reset when the player dies?
            ├─ yes ─► APawn / ACharacter
            └─ no  ─► APlayerController
```

When two answers seem right, prefer the longer-lived and more widely-visible class — moving
state *down* later is easy, moving it *up* after everything references it is not.

Still unsure? Reach out to a lead before building it.

---

## Supporting Unreal documentation

- [Gameplay Framework in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine) —
  the overview, and the source for the PlayerController replication and Pawn-lifetime statements
- [Game Mode and Game State](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine) —
  the server-only / fully-replicated split, and what belongs in each
- [APlayerState](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APlayerState) ·
  [UGameInstance](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameInstance?lang=en-US) ·
  [AGameSession](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/AGameSession) ·
  [ULocalPlayer](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ULocalPlayer)
- [Programming Subsystems](https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-subsystems-in-unreal-engine) —
  why a subsystem beats bolting members onto GameInstance
- [Client-Server Model](https://dev.epicgames.com/documentation/en-us/unreal-engine/client-server-model?application_version=4.27) —
  the authority model all of the above rests on
- [Travelling in Multiplayer](https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine) —
  seamless vs non-seamless, and which actors can be carried over
- [UWorld::SeamlessTravel](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UWorld/SeamlessTravel) ·
  [APlayerState::SeamlessTravelTo](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/APlayerState/SeamlessTravelTo) ·
  [APlayerState::CopyProperties](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/APlayerState/CopyProperties)

> The exact travel behaviour in the level-change table was read from the 5.7 engine source
> (`AGameModeBase::GetSeamlessTravelActorList`, `APlayerController::SeamlessTravelFrom`,
> `APlayerState::Reset`), since the published docs do not spell out the ordering that causes the
> `Score` reset.
