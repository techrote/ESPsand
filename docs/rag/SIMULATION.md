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

Important shared choices remain:

- water: gravity material, density 120, high gravity/lateral mobility;
- oil: gravity material, density 80, somewhat less mobile than water;
- lava: gravity material, density 150, deliberately viscous/slow;
- sodium-like: gravity-driven particle material, density 180;
- steam/smoke/fire: low-density buoyant materials;
- crust/wall: blocking static materials.

These are stylized tiny-grid parameters, not physical densities, viscosities or temperatures.

## PRNG and deterministic state

`Model` owns explicit PCG32 state. No core material/scene rule may draw from hidden/global entropy, `rand()`, timestamps or hardware noise. The same seed, initial/reset state and normalized per-tick input sequence must produce the same state trace.

ES-006 transport uses a deterministic tick/cell phase schedule rather than consuming PRNG draws for ordinary mobility. Product scenes use the model-owned PCG32 only for explicit scene placement/injection/selection rules, so all stochastic-looking scene behavior remains replayable.

## Fixed-step model and lifecycle

`ModelConfig` contains seed, scene ID and per-tick event/reaction budget limits. `Model` provides `init`, `reset`, `reseed`, `step`, state inspection, invariant checks and `state_hash`.

Hardware/runtime code schedules ticks and constructs normalized `InputFrame` values before crossing the pure-model boundary. Sensor polling cadence and wall-clock jitter are not simulation semantics.

Available model scenes are now:

- `kDeterminismFixture` — original ES-004 PRNG/input/hash fixture;
- `kDynamicsFixture` — ES-006 shared-mechanics substrate fixture;
- `kLavaWater` — product scene 1, ES-007;
- `kSodiumWater` — product scene 2, ES-008;
- `kOilFire` — product scene 3, ES-008.

The two fixtures remain non-product test substrates. The stable product order is encoded centrally as Lava + Water -> Sodium-like + Water -> Oil + Fire -> Lava + Water.

## Bounded per-tick work

`WorkBudget` supplies fixed-width event/reaction counters with atomic all-or-nothing consumption and saturating dropped-work counters. The model resets both every tick.

ES-006 reaction candidates consume reaction budget before product commit and never recurse. Optional local reaction impulse consumes event budget. Transport/heat/lifecycle passes remain fixed scans of the 256-cell world.

Product-scene touch actions also consume event budget. Autonomous injections are single bounded attempts at their scheduled ticks and never create queues/backlogs. Shared reactions remain the only reaction executor.

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

`kDynamicsFixture` adds `kDynamicsSchemaVersion`. Each product scene includes that shared dynamics discriminator plus its own scene schema version. ES-008 adds `kSodiumWaterSceneSchemaVersion` and `kOilFireSceneSchemaVersion` without renumbering earlier scene IDs. Per-tick scene action stats are included only for the active scene.

## Transport — ES-006 baseline

`DynamicsEngine` performs one deterministic bounded transport pass per model tick.

- normalized gravity selects a deterministic cardinal movement axis; diagonal input becomes a deterministic X/Y duty sequence;
- gravity materials move with gravity; steam/smoke/fire move opposite gravity;
- dynamic cells move into empty space or density-swap with non-blocking materials when ordering permits;
- liquids/gases may attempt perpendicular relaxation when primary movement is blocked;
- signed spin biases lateral side choice;
- shake/motion/tap/absolute spin provide bounded mobility disturbance but do not replace gravity;
- each cell participates in at most one transport exchange in the pass.

Whole-cell moves/swaps conserve tracked material mass exactly. Scene injection intentionally adds mass; reaction rules transform identity while preserving participating cell masses.

## Heat — ES-006 baseline

Each horizontal/vertical pair is visited once, with bounded integer exchange proportional to the lower material conductivity. Equal/opposite deltas accumulate in a fixed 256-element array before application; then material-specific ambient loss moves temperatures toward zero. Final values clamp to `int16_t`.

Units are game/simulation units chosen for bounded convergence and useful visible gradients, not physical temperature calibration.

## Reactions and finite fire — shared baseline

The centralized shared reaction table remains:

| Contact | Products | Baseline effect |
| --- | --- | --- |
| lava + water | crust + steam | heated persistent solid + hot buoyant gas |
| sodium_like + water | fire + steam | finite hot fire + hot gas |
| oil + fire | fire + fire | fuel cell becomes finite hot fire |

Each cell may participate in at most one adjacency reaction in a tick. Traversal is deterministic, every accepted candidate consumes reaction budget, and there is no recursive chain executor.

Fire is a shared finite material state. `aux` carries remaining lifetime; expired fire becomes smoke at reduced temperature while preserving mass. ES-008 relies on this generic lifecycle rather than implementing scene-local flame timers.

## Lava + Water — ES-007 product scene

`LavaWaterScene` is scene policy around the shared ES-006 engine. It initializes a substantial water reservoir, hot lava body and seeded near-contact cell, then periodically attempts bounded lava/water replenishment. Strong disturbance may relocate only a tiny capped number of existing crust cells, preserving mass while reopening contact surfaces. Slider/combo provide optional bounded injection under the accepted touch truth.

All flow, heat, gas and lava-water transformation still come from the shared dynamics engine.

## Sodium-like + Water — ES-008 product scene

`SodiumWaterScene` deliberately avoids scripted skitter/fizz animation.

### Initial/autonomous state

- border walls bound the world;
- water fills the lower six interior rows;
- a small finite set of sodium-like particles begins above/near the water, including one seeded near-contact placement;
- one sodium-like top injection is attempted every 180 ticks;
- one water refill attempt occurs every 300 ticks.

### Reaction behavior

When sodium-like contacts water, the centralized ES-006 rule produces finite fire plus steam. The ordinary reaction impulse writes bounded motion proxies to the product cells, so energetic local movement is a shared reaction consequence rather than arbitrary scene animation. Fire then expires through the generic fire-to-smoke lifecycle.

### Optional touch

- slider adds one sodium-like cell near the selected X, at the shared capped touch cadence;
- combo inserts one adjacent sodium-like/water pair into available space;
- both actions consume event budget;
- independent A/B zones remain disabled.

The scene remains complete through autonomous behavior + IMU alone.

## Oil + Fire — ES-008 product scene

`OilFireScene` proves finite fuel propagation/extinction using only shared material/reaction behavior.

### Initial state

- a lower water layer provides a density reference;
- an amber oil pool begins above it, using the shared oil density/mobility metadata;
- one seeded fire cell starts inside the fuel.

### Autonomous burn cycle

The first part of each 480-tick cycle intentionally contains no fuel injection, allowing fuel to be consumed and finite fire to gutter out. From phase 240 to before phase 300, sparse oil refill attempts occur every 12 ticks. At phase 300, one existing oil cell is re-ignited if available. The common oil/fire rule then handles subsequent propagation; generic fire lifetime handles extinction/smoke.

This creates a stateful build/burn/extinguish/refill/reignite arc without a permanent decorative flame.

### Optional touch

- slider adds one oil cell near the selected X;
- combo ignites one existing oil cell;
- propagation after ignition still belongs to `DynamicsEngine`;
- independent A/B zones remain disabled.

## Deterministic scene evidence — ES-008

ES-008 tests require:

- stable three-scene catalogue order and names;
- deterministic strong Sodium-like + Water initialization;
- finite sodium consumption, shared reaction products and bounded reaction impulse;
- bounded slider/combo actions;
- deterministic Oil + Fire initialization with oil above water;
- fuel consumption, finite fire expiry, smoke and a pre-refill extinction interval;
- fixed-seed duplicate-model traces for both new scenes;
- distinct initial renderer frames across all three product scenes;
- 1,000 randomized duplicate-model ticks for each new scene with state-hash equality, valid materials and bounded work.

## Safety semantics

Sodium-like and Oil + Fire are stylized visual/game simulations only. Do not encode real reactive-metal, fuel, ignition, quantity or experimental handling guidance.

## Tracer and biology — later milestones

Tracer remains static in ES-006 bulk transport until the tracer/plume milestone adds concentration behavior. Moss/plant and mite-like ecology remains the next hero milestone and must remain bounded/deterministic.
