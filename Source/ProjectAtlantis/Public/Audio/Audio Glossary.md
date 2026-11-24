# Audio Glossary and Reference

The terms used across `Source/ProjectAtlantis/Public/Audio` and `Private/Audio`, with the detail that doesn't fit in a two-sentence code comment. For MetaSound interfaces, see [MetaSound/Interfaces/README.md](MetaSound/Interfaces/README.md).

## How a fact travels

```
Gameplay  →  Front door  →  Data router  →  Listeners
             (validates)    (delivers)      (music, player audio, ...)
```

Gameplay never talks to a listener directly, and listeners never reference gameplay.

---

## Pipeline

**Fact**
: A piece of gameplay information sent to audio, such as "oxygen is at 0.4". It's a gameplay tag (what it is) plus a payload (its value). Fact tags live in `AtlantisAudioFactTags`.

**Front door** (`UAtlantisAudioDataBPFL::SetAudioDataPayload`)
: The only way to send a fact. It checks the fact against the payload registry, then hands it to the router. It returns false and logs an error if the fact is rejected, or if there's no world or router.

**Payload registry** (Project Settings > Game > Atlantis Audio Settings > Atlantis Audio Validation)
: Maps each fact tag to the payload struct it must carry. An unregistered tag, an empty struct or the wrong struct is rejected with an error. Add a row for every new fact tag.

**Payload shapes** (`AtlantisAudioPayloadTypes.h`)
: Generic containers whose meaning comes from the fact tag.

| Struct | Carries | Example |
|---|---|---|
| `FAtlantisAudioFloatPayload` | one number | `Audio.Fact.Oxygen.Level`, 0-1 |
| `FAtlantisAudioBoolPayload` | one yes/no | `Audio.Fact.Stealth.Hiding` |
| `FAtlantisAudioKeyedStatePayload` | a state for one instance | `Audio.Fact.Enemy.Status` |
| `FAtlantisAudioTagPayload` | one gameplay tag | `Audio.Fact.Environment.Room` |

**Keyed state**
: A fact about one of many instances, such as one enemy or one investigation. `Key` says which one, and `State` is an `Audio.State.*` tag. If `Key` is left empty, the front door fills in the calling actor's name. Set it yourself when reporting on something else, such as an investigation's title.

**Data router** (`UASGC_DataRouterSubsystem`)
: Delivers each fact to every listener subscribed to its tag. There's one per world, so subscriptions end with the level.
- **It doesn't remember facts.** A fact with no listener is dropped, and a listener only hears facts sent after it subscribes. Subscribe before any actor's `BeginPlay`, for example in a world subsystem's `OnWorldBeginPlay`.
- **Listeners must unsubscribe.** A listener destroyed without unsubscribing triggers an `ensure` the next time its tag is broadcast.
- **World subsystem listeners unsubscribe in `PreDeinitialize`, not `Deinitialize`.** Unreal deinitializes world subsystems in no fixed order, so the router may already have cleared its list. `PreDeinitialize` runs on all of them first, while the router still exists.

**Listener** (`IASGC_FactListenerInterface`)
: Anything that receives facts. Implement `OnGameplayFactReceived` (`_Implementation` in C++, or the event in Blueprint), then call `Subscribe` on the router.

---

## Bridges (gameplay → facts)

**Fact bridge** (`UAtlantisAudioFactBridgeComponent`)
: Sits on the player pawn and sends presence, oxygen, underwater and hiding facts. It binds once the pawn is locally controlled. It retries for up to 120 ticks (about 2 seconds) for the player state, then logs an error.
- **Initial facts:** on bind, it sends each fact's current value, so listeners are right from the start.
- **Reset facts:** on unbind (death, unpossession, level end), it sends `Critical = false`, `Hiding = false` and `Underwater = false`, then `Present = false`. Without these, listeners would keep the last value forever, and an alarm or the music's Tension would stay stuck.

**Underwater fact** (`Audio.Fact.Player.Underwater`)
: True while the player is in a water volume, from the movement component's `OnUnderwaterChanged`. Entering water is sent straight away. Leaving is sent only once the player has stayed out for **Underwater Exit Delay** (0.5 seconds by default, set on the bridge component), because bobbing at the surface crosses the volume's edge several times a second.
- **Oxygen.Critical stays true on land.** Oxygen stops draining out of water but doesn't refill, so Critical only clears once something restores oxygen. Suit sounds that should stop on land need **Underwater Only** ticked in `DT_PlayerAudio`.

**Presence fact** (`Audio.Fact.Player.Present`)
: True while a local player pawn is bound, and false when it's gone. It's sent before every other fact on bind, and after every reset on unbind. No player audio loop plays without it.

**Journal audio bridge** (`UAtlantisJournalAudioBridge`)
: Sends journal investigations as `Audio.Fact.Investigation.Status` keyed facts, using the investigation title as the key. It's a world subsystem rather than part of the fact bridge, because investigations belong to the game, not the pawn. So it never misses a change made while no pawn is possessed. When the journal clears, it re-sends every title it has sent as Undiscovered, because the journal is already empty by then.

---

## Music

**Music subsystem** (`UAtlantisAudioSubsystem`)
: Plays each level's music and drives it from facts. It's a game instance subsystem, so it outlives levels and has to track each world as it loads and unloads.
- `atlantis.audio.musicSubsystem`: `0` uses the Project Settings checkbox, `1` forces it off, `2` forces it on.
- `Atlantis.Audio.AxesSweepTest 1`: sweeps every axis 0 → 1 → 0, for testing a score.

**Music axes** (`Atlantis.Music.Axes` interface)
: The values the music's MetaSound reacts to, each from 0 to 1.
- **Tension:** pressure. Raised now by low and critical oxygen, hiding, an alerted or searching enemy, and an investigation in progress. Puzzles and low trust are planned (2026-11-09 music meeting).
- **PlayerState:** intensity. The mapping still has enemy rules for it, but the score doesn't use it, because the music is passive.
- **Environment:** shapes reverb, filtering and compression to match the space.

**Axis mapping** (`DA_AtlantisMusicAxisMapping`, set in Project Settings)
: The one asset for music tuning.
- **Baseline:** where an axis rests when no rule is pushing it.
- **Rule:** a fact tag that pushes an axis by an amount. **Tension adds every active rule together; PlayerState takes the strongest.** Both are clamped to 0-1.
- **Invert value:** for float facts, uses `1 - value`, so a lower number pushes harder (as with low oxygen).
- **Live tuning:** editing the asset during Play is heard straight away. It edits the real asset, so save or revert it afterwards.

**Layer splits** (`Atlantis.Music.Layers` interface)
: The axis value where a music layer comes in. `ActiveLayerSplit` is the PlayerState value where Ambient switches to Active. `PassiveLayerSplit` is the Tension value where Passive comes in. Raise a split to make that layer harder to reach.

---

## Rooms

**Room volume** (`UAtlantisRoomVolumeComponent`)
: Added to an Audio Gameplay Volume to say which room it is (a `Room.*` tag).
- **Priority** comes from the Reverb Volume Component on the same volume, so the room and the reverb always agree.
- **Don't disable a room volume during play.** If the listener is inside it, the music stays in that room.

**Room subsystem** (`UAtlantisRoomSubsystem`)
: Decides which room wins when volumes overlap, and sends `Audio.Fact.Environment.Room` only when the winner changes. The highest priority wins. On a tie, the most recently entered volume wins.
- **`Room.Default`** means the player isn't inside any volume. It isn't sent at the start, because the music already starts there.
- A room missing from the room type table logs an error and isn't sent.

**Room type table** (`DA_AtlantisRoomTypeTable`, set in Project Settings)
: Maps each `Room.*` tag to the integer sent to the MetaSound `RoomType` pin. **IDs must match the order of the room arrays in `MSP_AtlantisMusicEnvironment`.**

**Environment pins** (`Atlantis.Environment` interface)
: `RoomType` and `MaterialType` are integers. They change at volume boundaries or surface changes, not every frame. `MaterialType` matches Project Settings > Physics > Physical Surface, where `0` is Default.

---

## Player audio

**Player audio subsystem** (`UAtlantisPlayerAudioSubsystem`, #516, in progress)
: Plays the player's own suit sounds, such as breathing and the oxygen alarms, from the rows in `DT_PlayerAudio`. It's a world subsystem: it exists in every level without being placed, it outlives the pawn across respawns, and its sounds end with the level.
- **It remembers facts.** The router doesn't, so the subsystem keeps the last value of each fact it hears. That's how a loop started later gets the current oxygen level.
- **Loops only exist while they play.** An audio component is created when a loop starts and fades out (0.3 seconds) when it stops. Nothing sits playing silently.
- **Loops keep playing, muffled,** while the journal or pause menu pauses the game.

**Player audio table** (`DT_PlayerAudio`, set in Project Settings > Game > Atlantis Audio Settings > Atlantis Player Audio)
: One row per sound. A **Loop** row plays while all of these are true:
- **Present** is true.
- Its **Play While Fact** is true, such as `Audio.Fact.Player.Underwater` for breathing, or `Audio.Fact.Oxygen.Critical` for the alarm.
- The player is underwater, if **Underwater Only** is ticked.

**Parameter Fact** and **Parameter Name** (optional)
: Send a float fact's value to the MetaSound input with that name, on every change and as soon as the loop starts. For example, breathing sends `Audio.Fact.Oxygen.Level` to its `Oxygen` input. The name must match the input exactly, or nothing happens.

**One Shot rows**
: Not played yet. They're planned for sounds like the mask coming off.

**`SC_Player`**
: The sound class for the player's suit sounds. It's a child of `SC_SFX` and has its own copy of the User Main and User SFX volume buses. It isn't wired to `CB_World_LPF`, so the underwater low-pass skips it.

---

## Mix states

**Mix state subsystem** (`UAtlantisMixStateSubsystem`)
: Turns control bus mixes on and off from bool facts, using **Mix States** in Project Settings > Game > Atlantis Audio Settings. Each rule's mix is on while its **While Fact** is true.
- **Fades come from the mix.** Each stage's attack time sets how fast it comes in, and its release time sets how fast it goes out.
- **Mixes are turned off at level end.** Mixes live on the audio device, not in the level, so without this one would carry into the next level.
- Only bool facts drive mixes for now. Float facts, such as a value setting a bus directly, could be added as another kind of rule.

**Control bus, mix and destination**
: A **control bus** is a knob, such as `CB_World_LPF`. A **control bus mix** turns knobs to set values, with a fade. A sound class's **modulation destination** (Volume, Pitch, Highpass or Lowpass) is where a knob is wired in.
- **Sound classes don't inherit modulation.** Each class lists its own buses, so wire a bus into every class it should affect.
- On the LPF parameter, when two mixes set the same bus, the lower cutoff wins.

**Underwater low-pass** (`CB_World_LPF`, `CBM_Underwater`, #606)
: While `Audio.Fact.Player.Underwater` is true, `CBM_Underwater` lowers the cutoff on `SC_SFX` and `SC_Ambience`. Music, UI, dialogue and `SC_Player` aren't wired to the bus, so they stay clear.
