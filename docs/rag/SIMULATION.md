# ESPsand v0 simulation model

## Internal resolution — ES-004 baseline

The deterministic world is **16×16 logical cells** with a fixed capacity of 256 cells in row-major order. ES-005 deterministically aggregates this world to the physical 8×8 LEDs, preserving the intentional four-logical-samples-per-LED supersampling baseline.

A future resolution change is a model-semantic change and must preserve bounded storage, deterministic host tests and an acceptable firmware frame budget.

## Cell/state model

The common `Cell` remains an 8-byte trivially copyable value:

```text
MaterialId material    // uint8 stable registry ID
uint8      mass        // fill/mass proxy
int8       motion_x    // compact signed recent-motion proxy
int8       motion_y
int16      temperature // bounded temperature/energy proxy in model-defined units
uint8      aux         // material-specific compact state
uint8      flags
```

ES-006 activates motion/temperature without changing the layout. Successful transport records bounded signed recent motion that decays toward zero. Temperature participates in pairwise exchange and ambient loss. `aux` remains compact material-specific state; ES-006 uses it for finite fire lifetime.

World access is bounds-checked through `try_cell` / `set_cell`; iteration and hashing use deterministic row-major order. Material accounting reports cell counts, per-material mass, occupied cells, total mass and invalid-state diagnostics.

## Material registry and shared dynamics metadata

Material IDs remain the stable ES-004 set: empty, wall, crust, water, oil, lava, steam, smoke, fire, sodium-like, tracer and moss.

`kMaterialDynamics` remains the single shared source of transport kind, stylized density rank, gravity/lateral mobility, thermal conductivity, ambient loss and blocking semantics. Scene code must not introduce a second density/viscosity table.

The current important ordering/mobility choices remain:

- water: gravity material, density 120, high gravity/lateral mobility;
- oil: gravity material, density 80, somewhat less mobile than water;
- lava: gravity material, density 150, deliberately viscous/slow;
- steam/smoke/fire: low-density buoyant materials;
- crust/wall: blocking static materials.

These are stylized tiny-grid parameters, not physical densities, viscosities or temperatures.

## PRNG and deterministic state

`Model` owns explicit PCG32 state. No core material/scene rule may draw from hidden/global entropy, `rand()`, timestamps or hardware noise. The same seed, initial/reset state and normalized per-tick input sequence must produce the same state trace.

ES-006 transport uses a deterministic tick/cell phase schedule rather than consuming PRNG draws for ordinary mobility. ES-007 does use the model-owned PCG32 for scene placement/injection/fracture selection; those draws are therefore part of explicit deterministic scene evolution.

## Fixed-step model and lifecycle

`ModelConfig` contains seed, scene ID and per-tick event/reaction budget limits. `Model` provides `init`, `reset`, `reseed`, `step`, state inspection, invariant checks and `state_hash`.

Hardware/runtime code schedules ticks and constructs normalized `InputFrame` values before crossing the pure-model boundary. Sensor polling cadence and wall-clock jitter are not simulation semantics.

Available model scenes are now:

- `kDeterminismFixture` — original ES-004 PRNG/input/hash fixture;
- `kDynamicsFixture` — ES-006 shared-mechanics substrate fixture;
- `kLavaWater` — first product scene, added by ES-007.

The two fixtures remain non-product test substrates.

## Bounded per-tick work

`WorkBudget` supplies fixed-width event/reaction counters with atomic all-or-nothing consumption and saturating dropped-work counters. The model resets both every tick.

ES-006 reaction candidates consume reaction budget before product commit and never recurse. Optional local reaction impulse consumes event budget. Transport/heat/lifecycle passes remain fixed scans of the 256-cell world.

ES-007 scene-specific actions also use bounded work: slider injection and combo burst consume event budget, and a disturbance may attempt only a tiny fixed number of crust relocations. Autonomous periodic replenishment is one bounded injection attempt at its scheduled tick; it does not start a queue or backlog.

## State hashing and schema boundaries

State hashing uses versioned FNV-1a 64 over canonical explicit little-endian fields, not raw object memory. It is a replay/regression identity, not cryptographic integrity.

The original ES-004 exact golden trace remains unchanged:

```text
initial      0x4943A6C732CA020D
after tick 1 0x75A4B3C9249EF546
after tick 2 0xFC14CC3D7A9C7408
after tick 3 0xB411D621F3D3F1C6
after tick 4 0x8F0F30D22E87FB14
```

`kDynamicsFixture` adds `kDynamicsSchemaVersion` to its hash identity. `kLavaWater` includes both that dynamics schema and `kLavaWaterSceneSchemaVersion`, plus the scene's per-tick observable action stats. This isolates intentional later scene evolution from unrelated foundational fixtures.

## Transport — ES-006 baseline

`DynamicsEngine` performs one deterministic bounded transport pass per model tick.

- normalized gravity selects a deterministic cardinal movement axis; diagonal input becomes a deterministic X/Y duty sequence;
- gravity materials move with gravity; steam/smoke/fire move opposite gravity;
- dynamic cells move into empty space or density-swap with non-blocking materials when ordering permits;
- liquids/gases may attempt perpendicular relaxation when primary movement is blocked;
- signed spin biases lateral side choice;
- shake/motion/tap/absolute spin provide bounded mobility disturbance but do not replace gravity;
- each cell participates in at most one transport exchange in the pass.

Whole-cell moves/swaps conserve tracked material mass exactly. Scene injection intentionally adds mass; reaction rules transform identity while preserving the two participating cell masses.

## Heat — ES-006 baseline

Each horizontal/vertical pair is visited once, with bounded integer exchange proportional to the lower material conductivity. Equal/opposite deltas accumulate in a fixed 256-element array before application; then material-specific ambient loss moves temperatures toward zero. Final values clamp to `int16_t`.

Units are game/simulation units chosen for bounded convergence and useful visible gradients, not physical temperature calibration.

## Reactions — ES-006 baseline

The centralized shared reaction table remains:

| Contact | Products | Baseline effect |
| --- | --- | --- |
| lava + water | crust + steam | heated persistent solid + hot buoyant gas |
| sodium_like + water | fire + steam | finite hot fire + hot gas |
| oil + fire | fire + fire | fuel cell becomes finite hot fire |

Each cell may participate in at most one adjacency reaction in a tick. Traversal is deterministic, every accepted candidate consumes reaction budget, and there is no recursive chain executor.

## Lava + Water — ES-007 product scene

`LavaWaterScene` is scene policy around the shared ES-006 engine, not a bespoke physics implementation.

### Initial state

- border walls remain fixed;
- a 12×5 interior water reservoir provides a strong blue/cyan body;
- a 6×2 hot lava body begins above it;
- one seeded lava cell is placed immediately above the reservoir to guarantee early contact/reaction without waiting for a lucky random arrangement.

The starting layout consumes the model-owned seed only for that contact X position and therefore resets exactly for the same seed.

### Autonomous arc

- lava attempts one bounded top-interior injection every 48 model ticks;
- water attempts one bounded top-interior replenishment every 120 ticks;
- an injection only replaces empty/steam/smoke/fire space; it does not erase wall/crust/bulk liquid;
- ordinary gravity, heat and the centralized lava-water rule then determine contact, crust accumulation and steam motion.

This intentional replenishment means total world mass may rise at scheduled injections. Between external/autonomous injections, the shared transport/reaction behavior conserves tracked mass under its documented whole-cell rules.

### Interaction

- tilt affects the ordinary shared gravity vector;
- strong motion can relocate only 2 or 3 existing crust cells per tick, subject to event budget, preserving those cell masses while reopening contact surfaces;
- pinch slider can inject one lava cell near its horizontal position at most once per 12 ticks and only while active/strong enough;
- combo can insert one adjacent lava/water pair into available interior space; conversion still occurs through `DynamicsEngine` and its reaction budget.

Independent A/B touch zones are not part of this scene because physical ES-003A evidence rejected them.

### Deterministic scene evidence

ES-007 host tests require:

- deterministic initialization and exact same-seed reset;
- persistent crust + steam from autonomous contact;
- materially different geometry for different gravity directions;
- bounded mass-preserving crust fracture;
- bounded slider/combo input behavior using the shared reaction path;
- a scripted 96-tick duplicate-model state-hash trace with gravity changes, shake, tap, combo and slider events;
- a 1,200-tick randomized duplicate-model replay with valid materials and bounded work.

## Sodium-like safety semantics

The sodium-like material is visual/game physics only. Do not encode real-world reactive-metal experimental guidance.

## Tracer and biology — later milestones

Tracer remains static in ES-006 bulk transport until the tracer/plume milestone adds concentration behavior. Moss/plant and mite-like ecology remains later work and must remain bounded/deterministic.
