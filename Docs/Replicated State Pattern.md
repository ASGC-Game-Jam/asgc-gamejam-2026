# Replicated State Pattern

How Project Atlantis handles server-authoritative state that other systems need to react to.
The reference implementation is `AAtlantisPlayerState`. Read this before adding a replicated
property anywhere, and follow the same shape.

---

## The shape

Three layers, each with one job.

```
UPROPERTY(ReplicatedUsing = OnRep_X)   ← transport. the netcode moves the value.
        │
        ▼
UFUNCTION() void OnRep_X(T OldX)       ← hook. exactly one, called by the netcode.
        │
        ▼
FOnXChanged OnXChanged                 ← fan-out. open subscription list.
        │
        ├──► WBP_HealthBar
        ├──► damage flash VFX
        └──► audio cue
```

Reference implementation:
[`AtlantisPlayerState.h`](../Source/ProjectAtlantis/Public/Core/AtlantisPlayerState.h) ·
[`AtlantisPlayerState.cpp`](../Source/ProjectAtlantis/Private/Core/AtlantisPlayerState.cpp)

Five replicated properties, four change delegates, authority-guarded setters, and persistence
across seamless travel.

---

## Why it is shaped this way

**Nothing can bind to a RepNotify. That is the whole reason the delegate exists.**
A RepNotify is not a delegate. [`ReplicatedUsing` properties require you to provide a single
RepNotify function that is called on clients whenever the associated property is
replicated](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicate-actor-properties-in-unreal-engine) —
one property, one function, no subscriber list, and it is private besides. So the OnRep is the
*hook*, and the delegate is the *fan-out*. Without the delegate layer, every consumer would
have to poll each tick, or the PlayerState would have to hard-reference every widget and effect
that cares about it.

**The setter writes the value. The OnRep never does.**
By the time an OnRep runs on a client, the replication system has already written the property —
it writes first, then notifies. The OnRep only receives the *old* value: Epic's reference
confirms a RepNotify [may take one parameter of the same type as the property, into which the
replication system automatically passes the previous
value](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicate-actor-properties-in-unreal-engine).
There is nothing for it to assign. Meanwhile the server has no replication writing *into* it —
[property replication is server-to-client only](https://dev.epicgames.com/documentation/en-us/unreal-engine/property-replication-in-unreal-engine?application_version=5.2) —
so something has to author the value there, and that is the setter. Both paths write first,
then notify:

| | writes the property | calls the OnRep |
| --- | --- | --- |
| **Client** | the replication system | the replication system |
| **Server** | the setter | the setter, by hand |

**Each setter calls its own OnRep by hand, and that is not a hack.**
Replication [calls the RepNotify on clients](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicate-actor-properties-in-unreal-engine),
never on the authority. Skip the manual call and a listen-server host silently misses every side
effect its clients receive. Note that a bare C++ assignment (`OxygenCapacity = 50.f;`) fires
nothing at all locally — unlike Blueprint, where the Set node on a RepNotify variable does
invoke the notify. That mismatch is the most common bug when moving Blueprint logic into C++,
and Epic's own Blueprint-facing page phrases it in a way that reinforces the confusion — see the
warning at the end of this document.

**Delegates are grouped by what listens, not one per property.**
Five properties are served by four delegates: `bBallastAllocation` and `BallastState` share
`OnBallastChanged`, because nothing consumes one without the other. This is not only tidiness:
[there is no deterministic order between the RepNotify callbacks of different replicated
variables](https://dev.epicgames.com/documentation/unreal-engine/replicated-object-execution-order-in-unreal-engine),
so two values that must be consumed together have to arrive through one event.

Resist the temptation to collapse further into a single `OnStateChanged`. It sounds cheaper and
is not: every listener then wakes on every change and branches to work out whether it cares, so
an ammo change runs health-bar code. It also destroys the payload — listeners have to re-read
state and cache their own previous value just to compute a delta.

> **Merge** two properties into one delegate when a listener always wants both together.
> **Split** a delegate when a handler would need to switch on *what* changed.

**Dynamic delegates here, because the consumers are UMG.**
`BlueprintAssignable` requires `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*`. Epic states plainly that
[dynamic delegates can be serialized, their functions can be found by name, and they are *slower
than regular delegates*](https://dev.epicgames.com/documentation/en-us/unreal-engine/dynamic-delegates-in-unreal-engine) —
they route through `ProcessEvent`, which costs roughly an order of magnitude more than
non-dynamic dispatch. That is fine at PlayerState change frequency and buys direct binding from
widgets. Broadcasting costs nothing when nobody is listening: [it is always safe to call
`Broadcast()` on a multicast delegate, even if nothing is
bound](https://dev.epicgames.com/documentation/en-us/unreal-engine/multicast-delegates-in-unreal-engine),
which is why declaring an event no one has subscribed to yet is free. If only C++ ever listens, use
`DECLARE_MULTICAST_DELEGATE_*` and bind with `AddUObject`, keeping the `FDelegateHandle` to
unbind with — the same way
[`ScenePerformanceAnalyticsSubsystem`](../Source/ProjectAtlantis/Private/Analytics/ScenePerformanceAnalyticsSubsystem.cpp)
handles engine delegates.

**Properties are private and `BlueprintReadOnly`, never `BlueprintReadWrite`.**
A client writing a replicated property is overwritten on the next update anyway, and does it
while bypassing the authority check. Read-only properties plus guarded setters make the
authority rule structural instead of something everyone has to remember.

**Every property gets an explicit initializer.**
Initial replication compares the server value against the class default object. A value equal
to the default is never sent, so no initial OnRep fires on the client. Consumers must read
current state when they bind rather than wait for an event. If a property genuinely needs the
event regardless, register it with `DOREPLIFETIME_CONDITION_NOTIFY` and
[`REPNOTIFY_Always`, which executes the RepNotify every time the property replicates rather than
only when it changes](https://dev.epicgames.com/documentation/en-us/unreal-engine/conditional-property-replication-in-unreal-engine?application_version=5.2).

**`CopyProperties` is overridden, and deliberately fires no notifies.**
[`APlayerState::CopyProperties` exists to copy the properties that need to survive into an
inactive PlayerState](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/APlayerState/CopyProperties),
and [seamless travel carries persisting actors across to the new
level](https://dev.epicgames.com/documentation/en-us/unreal-engine/travelling-in-multiplayer-in-unreal-engine).
Without the override, custom state is silently lost. It assigns directly because it is
construction, not a gameplay change — broadcasting there would fire phantom "oxygen changed"
events at every level transition.

**Net update frequency is raised to 10 Hz.** `APlayerState` defaults to 1 Hz, which would delay a
change by up to a second. The engine's own guidance runs the other way — [reducing
`NetUpdateFrequency` on less-important or less-frequently-changing actors makes network updates
more efficient](https://dev.epicgames.com/documentation/en-us/unreal-engine/actor-priority-in-unreal-engine) —
so treat 10 Hz as a deliberate trade for oxygen's responsiveness, not a default to copy onto
every actor. `APlayerState` already enables replication and always-relevancy in its own
constructor, so neither is set again.

---

## How to interface with it

**Read a value.** All getters are `BlueprintPure`.

```cpp
const float Oxygen = PlayerState->GetOxygenCapacity();
const bool bAllocated = PlayerState->IsBallastAllocated();
```

**React to a change, from C++.** Bind on setup, unbind on teardown.

```cpp
if (AAtlantisPlayerState* PS = GetPlayerState<AAtlantisPlayerState>())
{
    PS->OnOxygenCapacityChanged.AddDynamic(this, &UMyWidget::OnOxygenChanged);
    OnOxygenChanged(PS->GetOxygenCapacity(), PS->GetOxygenCapacity()); // seed current state
}
```

**React to a change, from Blueprint.** Get Player State, cast to `BP_AtlantisPlayerState`,
then Bind Event to the dispatcher you want.

Either way: **bind, then immediately read the current value.** You have almost certainly missed
at least one event, and the initial one may never have fired at all.

**Change a value.** Server only, always through the setter.

```cpp
PlayerState->SetOxygenCapacity(80.f);   // no-ops entirely on a client
```

A client that needs to request a change goes through a `Server*` RPC on the PlayerController,
which then calls the setter on the authority. Never assign the property directly — you would
lose the notify on the server and the change-detection guard with it.

---

## Adding a new replicated property

The recipe, in order. Every step matters.

1. **Declare the property** — private, `ReplicatedUsing`, `BlueprintReadOnly`,
   `meta = (AllowPrivateAccess = "true")`, and an explicit initializer.
2. **Register it** with `DOREPLIFETIME` in `GetLifetimeReplicatedProps`. Miss this and it
   never replicates, with no warning.
3. **Declare the RepNotify** as `UFUNCTION() void OnRep_X(T OldX);`. The `UFUNCTION()` is
   mandatory — without it UHT never registers the function and the callback silently never
   fires, while the value replicates perfectly. This is the single most common mistake.
4. **Decide the delegate.** New event, or join an existing group? Apply the merge/split rule
   above.
5. **Broadcast from the OnRep, and do nothing else there.** Keep it `const` where possible;
   a const OnRep cannot accidentally write the property.
6. **Write an authority-guarded setter** that early-returns on `!HasAuthority()` and on an
   unchanged value, then writes, then calls the OnRep by hand.
7. **Add it to `CopyProperties`** if it should survive seamless travel.
8. **Add a `BlueprintPure` getter.**

Naming follows `Tech Standards.md`: `OnRep_Variable` (3.3.1.2), dispatchers prefixed `On`
(3.3.1.4), RPCs prefixed `Server`/`Client`/`Multicast` (3.3.1.5), booleans prefixed `b`
(3.2.1.3).

---

## Repeat this pattern

**Use it for** any server-authoritative state that more than one system reacts to — GameState
match data, ability and equipment components, objective progress, interactable actor state.
When only the owning class cares, skip the delegate and put the work in the OnRep body; add
the delegate the moment a second, unrelated system needs to know.

**Do not use it for:**

- **High-frequency data** — position, velocity, aim rotation. Do not broadcast per tick; let
  interested systems poll. Delegates are for discrete, infrequent transitions.
- **One-shot events with no persistent state** — explosions, hit sounds, tracers. Use a
  `Multicast` RPC. A RepNotify will not fire if the value did not change, and late joiners
  never see it at all.
- **Large arrays that change often.** A `TArray` resends in full on any change; reach for
  `FFastArraySerializer` instead. `EquippedItems` is a known instance of this and will need
  revisiting.

**Replicate state, RPC events.** Oxygen is a RepNotify. "Spawn a bubble burst here" is a
`MulticastSpawnBubbleBurst`.

---

## What will bite you

| Symptom | Cause |
| --- | --- |
| Value replicates, callback never runs | Missing `UFUNCTION()` on the OnRep |
| Nothing replicates at all | Missing `DOREPLIFETIME`, or actor does not replicate |
| Works on clients, broken for the host | Setter did not call the OnRep by hand |
| No event on join or first set | Server value equals the CDO default, so nothing was sent |
| Same value set twice, only one event | `REPNOTIFY_OnChanged` is the default; use `REPNOTIFY_Always` |
| State lost after a level transition | `CopyProperties` not overridden |
| Client change reverts a moment later | Client wrote a replicated property; needs a `Server*` RPC |
| Stale listeners after a widget closes | `AddDynamic` without a matching `RemoveDynamic` |
| Logic across two OnReps breaks intermittently | There is no deterministic order between different properties' RepNotifies |

That last one is worth stating plainly: **never write logic that depends on one property's
RepNotify running before another's.** The call order on the client bears no relation to
declaration order, memory layout, or the order the server marked them dirty. If two values must
be consumed together, put them behind one delegate — which is the real reason
`bBallastAllocation` and `BallastState` share `OnBallastChanged` rather than merely a tidiness
preference.

Not sure whether something belongs in replicated state or an RPC? Reach out to a lead before
building it — the two are hard to swap later.

---

## Supporting Unreal documentation

- [Replicate Actor Properties](https://dev.epicgames.com/documentation/en-us/unreal-engine/replicate-actor-properties-in-unreal-engine) —
  the primary reference. Covers `ReplicatedUsing`, `GetLifetimeReplicatedProps`, and confirms
  that a RepNotify may take one parameter of the same type as the property, into which the
  replication system passes the previous value automatically.
- [Conditional Property Replication](https://dev.epicgames.com/documentation/en-us/unreal-engine/conditional-property-replication-in-unreal-engine?application_version=5.2) —
  `DOREPLIFETIME_CONDITION_NOTIFY`, and the `REPNOTIFY_Always` vs `REPNOTIFY_OnChanged`
  distinction this doc relies on.
- [Replicated Object Execution Order](https://dev.epicgames.com/documentation/unreal-engine/replicated-object-execution-order-in-unreal-engine) —
  the source for the ordering warning above.
- [Multiplayer Programming Quick Start](https://dev.epicgames.com/documentation/unreal-engine/multiplayer-programming-quick-start-for-unreal-engine) —
  end-to-end walkthrough if you are new to replication.
- [Replicating Actor Components](https://dev.epicgames.com/documentation/unreal-engine/replicating-actor-components-in-unreal-engine) —
  the same pattern applied to components, which is where most gameplay state will end up.

On the delegate layer:

- [Multicast Delegates](https://dev.epicgames.com/documentation/en-us/unreal-engine/multicast-delegates-in-unreal-engine) —
  confirms it is always safe to `Broadcast()` even with nothing bound, and that multicast
  signatures cannot return a value.
- [Dynamic Delegates](https://dev.epicgames.com/documentation/en-us/unreal-engine/dynamic-delegates-in-unreal-engine) —
  states outright that dynamic delegates are serialisable, resolved by name, and *slower than
  regular delegates*, which is the basis for preferring non-dynamic wherever Blueprint does not
  need to bind.
- [Delegates and Lambda Functions](https://dev.epicgames.com/documentation/en-us/unreal-engine/delegates-and-lambda-functions-in-unreal-engine) —
  binding forms, `FDelegateHandle`, and lambda binding.

> ⚠️ Epic's older Blueprint-facing page
> [Variable Replication [RepNotify]](https://dev.epicgames.com/documentation/en-us/unreal-engine/1.4---variable-replication-%5Brepnotify%5D?application_version=4.27)
> says the RepNotify function "executes on both the Server and Client machines." That describes
> **Blueprint only**, where the Set node compiles to *assign, then call the notify*. In C++ a
> bare assignment fires nothing, and replication never calls a RepNotify on the authority —
> which is exactly why every setter in this pattern calls its own OnRep by hand. Do not read
> that page as describing C++ behaviour.
