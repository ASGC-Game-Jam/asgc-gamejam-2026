# Reacting to Player Movement

If you're working on Animation, Audio, UI or VFX and need to know what the player is doing (walking, swimming, jumping, landing), this is where to look. Everything below is already in the project; you don't need to add anything to the character to use it.

There are two ways to get the information:

- **Read it** when you need it (every frame, or whenever). Good for things that change all the time, like speed.
- **Listen to an event** that fires when something changes. Good for things that happen once, like entering water or landing.

---

## Traversal mode

The traversal mode tells you *how* the player is moving through the world, from FDS-001 Traversal:

| Value | Meaning |
| --- | --- |
| `None` | No pawn is possessed, or the engine is in a mode we don't use |
| `Terrestrial` | Walking, jumping or falling in a breathable area |
| `Swimming` | Moving through water |
| `SurfaceWalking` | Attached to an underwater walkable surface. Not produced yet, it comes with Sea Walk (#47) |

Walking and falling are both `Terrestrial`, so jumping does **not** change the traversal mode.

It lives on the **PlayerState**, not on the character:

- `Get Traversal Mode`: the current value.
- `On Traversal Mode Changed`: fires with the old and new mode whenever it changes.

In Blueprint: `Get Player State` -> `Cast To AtlantisPlayerState` -> bind to `On Traversal Mode Changed`.

**Always bind, then read the current value right away.** The first change happens while the player is spawning, usually before your widget or component exists, so if you only wait for the event you'll miss the starting mode.

Typical uses: switching ambience or music when the player goes in or out of water, swapping a HUD element, splash effects on entering water.

---

## Values to read

On the player character (`Get Player Character` -> `Cast To AtlantisPlayerCharacter`):

| Value | What it is |
| --- | --- |
| `Get Ground Speed` | Horizontal speed in cm/s, ignoring vertical movement |
| `Has Movement Input` | True while the player is trying to move, even if they're pushing against a wall |

On the character's movement component (`Get Character Movement`), from the engine:

| Value | What it is |
| --- | --- |
| `Is Falling` | In the air (jumping or falling) |
| `Is Moving On Ground` | Walking on the ground |

---

## Events

On the player character, from the engine:

| Event | Fires when |
| --- | --- |
| `On Jumped` | The player jumps |
| `On Landed` | The player lands after being in the air. Gives you the hit result, useful for landing dust or a thud |

Plus `On Traversal Mode Changed` on the PlayerState, described above.

---

## Not included (on purpose)

- **Footsteps.** These come from **anim notifies** placed on the walk/run animations, so the footstep plays exactly when the foot touches the ground. That's set up in the animations, not in the character code.
- **Floor surface type** (stone, sand, metal…). The project doesn't define any physical surface types yet (Project Settings -> Engine -> Physics -> Physical Surface). Once Audio or Art defines them, a getter for the surface under the player can be added so footsteps can change per surface. Ask in #240.

---

Missing something you need? Comment on #240 and I'll try to get on that ASAP or reach out to a lead
