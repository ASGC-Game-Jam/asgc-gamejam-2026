Original Document was written by Carlos Ortiz
A reference summary of key patterns and principles distinct to game development, alongside how general programming principles (OOP, SOLID, DRY) apply.

---

## Architectural Principles

**Data-Oriented Design (DOD)**
Organize code around how data is laid out and accessed in memory, not around conceptual objects. Optimizes for cache performance. Often means "struct of arrays" over "array of structs."
***Example:* store all bullet positions in one contiguous array and update them in a tight loop, instead of each `Bullet` object holding its own position scattered across the heap.**
***Use when:* large collections are processed every frame (particles, thousands of entities) and cache misses are the bottleneck.**

**Entity-Component-System (ECS)**
Entities are IDs, Components are pure data, Systems are logic operating over components. Decouples data from behavior; enables cache-friendly batch processing.
***Example:* a `Position` and a `Velocity` component plus a movement system that iterates every entity having both (the model used by Unity DOTS and Bevy).**
***Use when:* entities vary widely in composition and you want to add or remove behaviors at runtime without deep class trees.**

**Composition over Inheritance**
Build entities by assembling small, focused components rather than deep inheritance hierarchies. Avoids the rigid, brittle class trees that games are especially prone to.
***Example:* instead of `Player extends Character extends Entity`, give the player a `HealthComponent`, an `InputComponent`, and a `SpriteComponent` you can attach to any actor.**
***Use when:* your class tree starts forking to combine traits (FlyingEnemy vs SwimmingEnemy vs FlyingSwimmingEnemy).**

**Don't Repeat Yourself (DRY)**
Don't repeat code, or assets. If you find yourself hitting copy and paste, stop, pause, and think about another way that doesn't require you to copy paste. Not sure how? Reach out to a lead!
***Example:* keep weapon damage values in one data table that every weapon reads, rather than hard-coding the number in each weapon script.**
***Use when:* always, but especially the moment you reach for copy-paste on logic or asset config.**

**Single Responsibility Principle**
Keep all actors, classes, and functions limited to one single responsibility. That is, each has "one and only one job." Put another way: always avoid complex inputs and outputs.
***Example:* a `PlayerInput` class that only turns key presses into intent, with a separate `PlayerMovement` that consumes that intent and moves the body.**
***Use when:* you need "and" to describe a class or function (handles input AND saves files); split it.**

**Dependency Inversion**
A software design methodology used to reduce tight coupling between code layers. It states that high-level modules (business logic) should not depend directly on low-level modules (implementation details); instead, both should rely on abstractions (e.g., interfaces).
***Example:* gameplay code depends on an `IAudioService` interface; `FmodAudioService` implements it for the shipping game while `NullAudioService` stands in for tests. (See Dependency Injection and Inversion of Control below for how the concrete one gets supplied.)**
***Use when:* a high-level system would otherwise hard-reference a concrete subsystem you may swap or mock (e.g., a specific audio backend).**

**Inversion of Control (IoC)**
The framework, not your code, owns the flow: it creates your objects, decides when to call them, and manages their lifetime. You write the pieces; something above you wires them together and drives them. "Don't call us, we'll call you." Dependency Injection is one way to achieve IoC; an engine's lifecycle callbacks are another. Note the distinction: Dependency Inversion (DIP) is about *which direction dependencies point* (toward abstractions), while IoC is about *who controls execution and creation*.
***Example:* Unity calls your `MonoBehaviour.Start()` and `Update()`, and Unreal calls `BeginPlay()` and `Tick()` - you never call them yourself. The engine controls when your code runs.**
***Use when:* you are plugging into an engine or framework that wants to own creation and the update/lifecycle, rather than running `main()` yourself.**

**Dependency Injection (DI)**
Supply a class's dependencies from the outside (usually through its constructor) instead of letting it construct them or look them up itself. Makes dependencies explicit in the signature, easy to swap, and easy to fake in tests. It is the practical, push-based way to honor Dependency Inversion - and the opposite of a Service Locator, which the class pulls from globally.
***Example (constructor injection):***
```cpp
// Player depends on the abstraction, handed in from outside.
class Player {
public:
    explicit Player(IAudioService& audio) : audio(audio) {}
    void Jump() { audio.Play("jump.wav"); }
private:
    IAudioService& audio;
};

// A single "composition root" decides the concrete implementation:
FmodAudioService fmod;
Player player(fmod);            // real game
Player testPlayer(mockAudio);   // test - Player itself never changes
```
**Contrast with the locator anti-pattern, where `Player` fetches it itself (`audio = ServiceLocator::GetAudio();`) - that hides the dependency and is harder to test.**
***Use when:* a class needs a collaborator you may want to swap, mock, or configure (services, repositories, strategies).**

**Prototypal Data Modeling**
Define entities as data that inherits from a "prototype" entity rather than duplicating shared values. A goblin archer and a goblin wizard can both delegate to a base goblin, overriding only what differs; any field they omit falls back to the prototype. A great fit for one-off specials (bosses, unique items) - clone an existing entity in data and refine it. Keeps content DRY without writing a new class per variant.
***Example:***
```json
{ "id": "goblin",        "health": 20, "attack": 4 }
{ "id": "goblin-archer", "prototype": "goblin", "attack": 6, "ranged": true }
```
**The archer inherits `health` from `goblin` and overrides only `attack`.**
***Use when:* you have many near-identical content entries and want designers to define one-off specials by tweaking a base in data.**

---

## Structural Patterns

**Game Loop**
The core cycle: process input, update state by a time delta, render, repeat. Foundational and unique to games.
***Example:***
```
while (running) {
    processInput();
    update(deltaTime);
    render();
}
```
***Use when:* always for a real-time game; it is the backbone every other system hangs off.**

**Update Method**
Each game object has its own `update()` called once per frame, letting it advance its own behavior one tick at a time.
***Example:* `for (Entity& e : entities) e.update(dt);` - each enemy, projectile, and pickup advances itself.**
***Use when:* many objects each advance their own behavior per frame (enemies, projectiles, animations).**

**Fixed vs. Variable Timestep**
Decouple your update rate from your render rate. Fixed timestep keeps physics deterministic and stable; variable rendering runs as fast as the hardware allows.
***Example:***
```
accumulator += frameTime;
while (accumulator >= FIXED_DT) { stepPhysics(FIXED_DT); accumulator -= FIXED_DT; }
render(accumulator / FIXED_DT); // interpolate the leftover
```
***Use when:* physics or networked simulation must stay deterministic regardless of frame rate while visuals render as smoothly as possible.**

**Double Buffer**
Keep two copies of state, one being read (current) and one being written (next), then swap them atomically so a series of sequential changes appears instantaneous. Classic for rendering (never show a half-drawn frame) and for any simulation step where every entity should react to the previous frame's state rather than each other's half-updated state. Think of a theater swapping between two stages so the audience never sees the next scene being set up.
***Example:* Conway's Game of Life - compute the next grid from the current one, then swap, so no cell is read after it has already changed this tick.**
***Use when:* readers must never see partially-updated state (rendering a frame, or a simulation step where all cells update from the prior state).**

---

## Behavioral Patterns

**Command**
An action reified as an object: a method call you can store, queue, pass around, and reverse. Decouple input from what it triggers by binding buttons to command objects instead of hard-coded calls, which makes controls rebindable and lets the same command drive a player or an AI. Give a command an `undo()` alongside `execute()` and a history list, and you get multi-level undo and replays for free (store the commands, not full game-state snapshots).
***Example:* a `MoveCommand` with `execute()` (move by delta) and `undo()` (move by -delta); bind it to a key, push executed commands onto a stack, and pop to undo.**
***Use when:* you need rebindable controls, input recording/replay, undo/redo, or to let AI and players drive the same actions.**

**State Machine (and Hierarchical State Machines)**
Model entity behavior as discrete states with defined transitions instead of a tangle of boolean flags. Ubiquitous for AI, animation, and input handling. Three extensions worth knowing:
- Concurrent state machines: track independent aspects (e.g., movement vs. weapon) as separate machines, so you need n + m states instead of n x m.
- Hierarchical (HSM): states inherit from superstates; an unhandled input rolls up the chain, so shared behavior lives in one place.
- Pushdown automata: keep a stack of states so you can push a transient state (e.g., firing) and pop back to whatever you were doing, avoiding "firing-while-running" style combos.
***Example:* a guard with `Patrol -> Chase` (player spotted) `-> Attack` (in range) `-> Flee` (low health), each state owning its own update and transition checks.**
***Use when:* an entity has a handful of distinct modes with clear transitions (AI patrol/chase/flee, idle/jump/attack, menu flow).**

**Type Object**
Make entity "types" data instead of code. Rather than subclassing Monster into Dragon, Troll, and so on, keep one Monster class plus a Breed object holding the shared stats, and have each monster reference its breed. Designers can then add hundreds of variants by editing config files with no recompile. Pairs naturally with Prototypal Data Modeling above.
***Example:* `monster.breed->maxHealth` reads the shared stat; spawning a "Fire Drake" just loads a new `Breed` row, no new class.**
***Use when:* designers need to author many "kinds" of a thing (monster breeds, item types) from data without a programmer adding a subclass each time.**

**Subclass Sandbox**
Concentrate coupling in a base class. The base exposes a handful of safe, high-level operations (`playSound()`, `spawnParticles()`, `move()`) plus an abstract hook, and subclasses build behavior only from those operations. Ideal when you have many similar subclasses (e.g., 100 superpowers) that would otherwise each reach independently into audio, physics, and rendering.
***Example:* a `SkyLaunch` power implements `activate()` purely as `playSound("whoosh"); spawnParticles(); move(0, 20);` - never touching the audio or physics engines directly.**
***Use when:* many subclasses of one base (dozens of abilities) all poke the same subsystems and you want that coupling in one safe place.**

**Bytecode**
Encode behavior as data interpreted by a small virtual machine rather than compiled into the engine. Enables hot iteration, safe sandboxing of user or modder content, and behavior shipped as separate data files (e.g., spells). Powerful but heavy: you must build a VM and authoring tooling, it runs slower than native, and it is hard to debug. Skip it unless you genuinely need data-defined behavior at scale - overkill for a game jam.
***Example:* a heal spell stored as a token stream a tiny stack VM runs: `LITERAL 10`, `GET_HEALTH`, `ADD`, `SET_HEALTH`. Designers edit the script; the engine never recompiles.**
***Use when:* you need large amounts of safe, hot-reloadable designer/mod behavior and can afford a VM plus tooling (rarely worth it for a jam).**

---

## Performance Patterns

**Flyweight**
Share the data that is identical across many instances (intrinsic state: mesh, texture, terrain properties) and store only what varies per instance (extrinsic state: position, scale, color). Lets you render a forest of thousands of trees from a single shared model. Possibly the only Gang of Four pattern with direct hardware support - GPU instanced rendering is exactly this.
***Example:* 10,000 trees each store only a transform and reference one shared `TreeModel` (mesh + textures), drawn in a single instanced draw call.**
***Use when:* you render or store huge numbers of near-identical objects (forest of trees, tile terrain, sprites) and memory or GPU bandwidth is tight.**

**Object Pooling**
Pre-allocate and reuse objects instead of constant allocate/free churn. Reduces fragmentation and garbage-collector pressure in the hot path.
***Example:* a `BulletPool` of 500 pre-created bullets; firing grabs an inactive one and resets its fields instead of calling `new Bullet()` every shot.**
***Use when:* you spawn and destroy short-lived objects rapidly (bullets, particles) and allocation churn or GC spikes hurt the frame budget.**

**Spatial Partitioning**
Organize objects by location (grids, quadtrees, octrees, BSP trees) so you only check nearby objects for collisions, visibility, etc., instead of everything.
***Example:* a uniform grid where each cell lists the units inside it; collision only compares units in the same and neighboring cells, not all pairs.**
***Use when:* you run many position-based queries (collision, "what is near me", visibility) over enough objects that all-pairs checks are too slow.**

**Dirty Flag**
Defer expensive recalculation until something actually changes, marking state "dirty" and recomputing only when needed (e.g., transforms, derived data).
***Example:* cache a node's world transform; set `dirty = true` when its local transform or parent moves, and recompute only on the next read.**
***Use when:* a derived value is expensive to compute but changes rarely relative to how often it is read (world transforms, cached layout).**

**Data Locality**
Arrange data so things processed together live together in memory. The practical application of DOD at the code level.
***Example:* store `positions[]`, `velocities[]`, and `healths[]` as parallel arrays so a system streams through one array at a time, instead of an array of fat `Entity` structs.**
***Use when:* a hot loop stalls on cache misses and you can reorder data so items processed together sit together in memory.**

---

## Decoupling Patterns

**Event Queue / Message System**
Decouple senders from receivers by queuing events for later processing. Unlike Observer (which fires synchronously), a queue also decouples timing: the sender does not block on the handler, which smooths spikes and plays well with threading. Common for input, audio, and game-wide signals.
***Example:* play-sound requests get pushed to an audio queue and drained once per frame, so 50 explosions in one tick do not each stall the caller.**
***Use when:* senders should not block on or know about handlers, and events can be processed later or on another thread (input, audio triggers, achievements).**

**Observer**
Let objects subscribe to events from a subject without tight coupling. Caveats to watch: it is synchronous (a slow observer blocks the subject), it hides control flow (you cannot tell from the code who is listening), and it risks the "lapsed listener" leak where subjects keep references, so observers must unregister or they never get freed. Modern code often favors function or closure observers over full interfaces. If you also need to decouple timing, reach for an Event Queue.
***Example:* an achievement system subscribes to `OnEnemyKilled`; combat code fires the event without knowing achievements (or the UI, or the audio) are listening.**
***Use when:* one change should notify several interested parties synchronously and you do not need timing decoupling (UI reacting to a stat change).**

**Service Locator**
Provide global access to a service (audio, logging) without hardcoding the concrete implementation. Use sparingly - it is essentially controlled global state, and it carries a temporal-coupling risk (things must be registered before they are used). Two handy tricks: return a null service (a no-op) instead of crashing when nothing is registered, and wrap the real service in a logging decorator to trace calls while debugging. Often called a "provider" or "registry," but note the difference: a DI provider *hands* a dependency to a class (injection / push), whereas a locator makes the class *fetch* it globally (pull). The locator hides dependencies; injection makes them explicit.
***Example:* `AudioService* a = ServiceLocator::GetAudio(); a->play("hit.wav");` - callers ask the locator instead of holding an injected reference.**
***Use when:* a genuinely singular facility (audio device, logger) is needed widely and threading it through every call would be noise.**

**Component (as a decoupling pattern)**
Split a monolithic entity into independent domains (physics, rendering, AI) that can be mixed and matched.
***Example:* a `Door` entity composed of a `MeshComponent`, a `ColliderComponent`, and an `InteractableComponent` - reuse the same components on a chest or a lever.**
***Use when:* an entity class balloons with unrelated concerns (physics + rendering + AI) and you want to split and recombine them.**

**Singleton (use with caution)**
Tempting for "managers," but a singleton is just global state wrapped in a class: it couples unrelated systems, makes code hard to reason about, and forces lazy initialization you often do not want. Prefer passing the dependency in, reaching it through an object you already have (e.g., the Game), or a Service Locator when you truly need global access. The need for a single instance and the need for global access are separate problems - do not let the pattern force both on you.
***Example:* instead of `GameManager::Instance()` sprinkled everywhere, pass the `Game` object (or just the one service a system needs) into that system's constructor.**
***Use when:* almost never; only if a single instance plus global access is truly unavoidable and lighter options do not fit.**

**Unreal Subsystems (engine example)**
Engine-managed singletons whose lifetime is bound to an outer: Engine, GameInstance, World, or LocalPlayer. The engine instantiates them automatically, calls `Initialize()` / `Deinitialize()`, and you fetch them with `GetSubsystem<T>()`. They are primarily an Inversion of Control mechanism (the engine owns creation and lifecycle) plus a Service Locator (global typed lookup) - not Dependency Inversion by default, because callers name the concrete subsystem type. Route access through a `UInterface` if you want true DIP.
***Example:* `USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();` - the engine created and owns `USaveSubsystem`; your code just asks for it and never manages its lifetime.**
***Use when:* you need a modular, always-available system tied to a clear scope (a save system on the GameInstance, a day/night manager on the World) without writing singleton boilerplate or bloating GameMode.**

---

## Cross-Cutting Concerns

**Determinism**
Same inputs produce same outputs. Essential for replays, lockstep multiplayer, and debugging. Drives caution around floats, randomness, and iteration order.
***Example:* a seeded PRNG plus a fixed timestep means a recorded list of inputs replays the identical match every time.**

**Frame Budget Thinking**
You have ~16.6ms per frame at 60fps (~8.3ms at 120fps). Shapes nearly every performance decision.
***Example:* if physics eats 4ms and rendering 9ms, only ~3ms is left for game logic at 60fps before you drop a frame.**

---

## General Principles (Glossary)

**OOP (Object-Oriented Programming)**
A paradigm that bundles data and the behavior that acts on it into objects, organized through encapsulation (hide internal state behind an interface), inheritance (derive specialized types from general ones), and polymorphism (one interface, many implementations). Great for high-level gameplay logic and tooling; in hot paths it competes with DOD/ECS, which deliberately split data from behavior.
***Example:* an `Inventory` class hides its item list and exposes `addItem()` / `removeItem()`; callers never touch the underlying array.**

**SOLID**
Five object-oriented design guidelines for keeping code flexible and maintainable:
- **S - Single Responsibility** - a class or function has one and only one reason to change (see Single Responsibility Principle above).
- **O - Open/Closed** - open for extension, closed for modification; add behavior by extending, not by editing existing, working code.
- **L - Liskov Substitution** - a subtype must be usable anywhere its base type is expected, without surprising the caller.
- **I - Interface Segregation** - prefer several small, focused interfaces over one fat one, so clients depend only on what they use.
- **D - Dependency Inversion** - depend on abstractions, not concrete implementations (see Dependency Inversion above).

---

## How General Principles Apply

DRY, SOLID, and OOP still apply in games, but more selectively. DOD and ECS push back on OOP's "bundle data with behavior" premise where performance matters, so they coexist:

- **OOP** - High-level gameplay logic and tooling
- **DOD / ECS** - Performance-critical systems

The meta-principle: treat these as tools matched to specific constraints (performance, scale, determinism) rather than universal commandments.

---

## Further Reading

- **Game Programming Patterns** - Robert Nystrom (free at [gameprogrammingpatterns.com](https://gameprogrammingpatterns.com/contents.html)). The best documented home for most of these patterns.
- **Game Engine Architecture** - Jason Gregory. Comprehensive textbook.
- **Data-Oriented Design** - Richard Fabian (free at [dataorienteddesign.com](https://www.dataorienteddesign.com/)).
- **Data-Oriented Design and C++** - Mike Acton, CppCon 2014 talk. The canonical DOD reference.
