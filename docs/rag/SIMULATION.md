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

ES-006 activates the motion and temperature fields without changing the cell layout. A successful transport move records a bounded signed motion vector which decays toward zero on later ticks. It is deliberately a cheap recent-motion proxy rather than a continuous velocity field or CFD state. Temperature participates in local pairwise exchange and ambient loss. `aux` remains bounded material-specific state; ES-006 uses it for finite fire lifetime while preserving its future use for concentration, fuel, biomass or age.

World access is bounds-checked through `try_cell` / `set_cell`; iteration and hashing use deterministic row-major order. Material accounting reports cell counts, per-material mass, occupied cells, total mass and invalid-state diagnostics.

## Material registry

Material IDs remain the stable ES-004 identity set:

| ID | Name | Category |
| ---: | --- | --- |
| 0 | empty | empty |
| 1 | wall | solid |
| 2 | crust | solid |
| 3 | water | liquid |
| 4 | oil | liquid |
| 5 | lava | liquid |
| 6 | steam | gas |
| 7 | smoke | gas |
| 8 | fire | energy |
| 9 | sodium_like | particle |
| 10 | tracer | scalar |
| 11 | moss | biomass |

Material names are expressive game/simulation names, not claims of chemical fidelity. Mite-like mobile agents remain a likely separate bounded structure rather than a dense material ID.

### ES-006 dynamics metadata

`kMaterialDynamics` is the centralized dynamics table. Scene code must not grow its own density, viscosity or gas-ordering conditionals.

| Material | Transport | Density rank | Gravity mobility | Lateral mobility |
| --- | --- | ---: | ---: | ---: |
| empty | static/displaceable | 0 | 0 | 0 |
| wall | blocking static | 255 | 0 | 0 |
| crust | blocking static | 240 | 0 | 0 |
| water | gravity | 120 | 255 | 224 |
| oil | gravity | 80 | 208 | 176 |
| lava | gravity | 150 | 112 | 72 |
| steam | buoyant | 12 | 255 | 224 |
| smoke | buoyant | 8 | 224 | 208 |
| fire | buoyant | 4 | 248 | 224 |
| sodium_like | gravity | 180 | 224 | 104 |
| tracer | static in ES-006 | 96 | 0 | 0 |
| moss | blocking static in ES-006 | 160 | 0 | 0 |

These ranks and mobility values are stylized tiny-grid parameters, not physical densities or viscosities. Water is deliberately more mobile than oil, and lava is deliberately much more viscous. Thermal conductivity and ambient-loss coefficients are also centralized per material.

## PRNG and deterministic state

`Model` owns an explicit PCG32 generator. The default stream selector is 54 (stored internally as odd increment 109); seeding follows the standard two-step PCG initialization. PRNG state and increment are exposed for deterministic inspection and included in state hashing.

No core material/scene rule may draw from hidden/global entropy, `rand()`, timestamps or hardware noise. The same seed, initial/reset state and normalized per-tick input sequence must produce the same state trace.

ES-006 transport itself uses a deterministic tick/cell phase schedule for mobility rather than consuming random draws merely to decide whether a cell moves. This keeps transport replay independent of unrelated future PRNG consumers. A later rule may use the model-owned PRNG where actual stochastic behaviour materially improves the scene, but the draw must be explicit and fixture-tested.

## Fixed-step model and lifecycle

`ModelConfig` contains the explicit seed, scene ID and per-tick event/reaction budget limits. `Model` provides:

- `init(config)` — install configuration and initialize deterministic state;
- `reset()` — restore tick zero and the configured seed/initial scene state;
- `reseed(seed)` — replace the configured seed and reset deterministically;
- `step(InputFrame)` — execute exactly one logical simulation tick;
- state inspection, invariant checks and a deterministic state hash.

Hardware/runtime code is responsible for scheduling a fixed number of logical ticks and constructing normalized `InputFrame` values before crossing the pure-model boundary. Sensor polling cadence and wall-clock jitter are not simulation semantics.

Two non-product fixture scenes now exist:

- `kDeterminismFixture` is the original ES-004 marker/input/PRNG fixture and retains its locked golden trace unchanged;
- `kDynamicsFixture` is an ES-006 substrate fixture containing representative water/oil, gas/liquid and energetic/reactive contacts so deterministic motion-input traces exercise the shared engine without pretending to be a polished hero scene.

## Bounded per-tick work

`WorkBudget` supplies fixed-width event/reaction counters with atomic all-or-nothing consumption. The model resets them each tick.

ES-006 reaction candidates consume one reaction-budget unit before any products are committed. Exhausted candidates are skipped and counted as dropped work; there is no recursive reaction execution. Each applied reaction may also consume one event-budget unit for a bounded local motion impulse. Product transformation is not rolled back merely because the optional impulse budget is exhausted.

Transport, heat and lifecycle passes are fixed scans over the 256-cell world. No hot-path container grows with simulation activity.

## State hashing and trace fixtures

State hashing uses versioned FNV-1a 64 over explicitly serialized little-endian fields rather than object memory. It covers seed/configuration, tick, PRNG state, fixture state, work counters and every cell.

It is a **regression/replay identity**, not a cryptographic integrity guarantee. The hash schema is versioned. The ES-006 dynamics fixture additionally includes `kDynamicsSchemaVersion` in its hash identity because its future evolution depends on the shared dynamics rule set. The original ES-004 fixture omits that new field and therefore retains its existing locked hashes.

The locked ES-004 trace remains:

```text
initial      0x4943A6C732CA020D
after tick 1 0x75A4B3C9249EF546
after tick 2 0xFC14CC3D7A9C7408
after tick 3 0xB411D621F3D3F1C6
after tick 4 0x8F0F30D22E87FB14
```

ES-006 adds repeated duplicate-model motion traces rather than replacing the foundational fixture. Intentional changes to shared dynamics semantics must update the corresponding dynamics tests/schema deliberately.

## Transport — ES-006 baseline

`DynamicsEngine` performs one deterministic bounded transport pass per model tick.

- normalized gravity selects one cardinal movement axis each tick; diagonal gravity is represented over time by deterministic X/Y duty weighting rather than floating-point position integration;
- gravity-driven materials move with gravity; steam, smoke and fire move opposite gravity;
- a dynamic cell can move into empty space or swap with a non-blocking material when density ordering permits it;
- water/oil/lava/sodium-like movement rates are controlled by centralized mobility values;
- liquids and gases may try one perpendicular relaxation/dispersion move when their primary direction is blocked;
- signed normalized `spin_rate` biases which perpendicular side is tried first;
- `shake_energy`, `motion_energy`, `tap_impulse` and absolute spin contribute only to a bounded disturbance/mobility boost. They do **not** replace or rotate the low-frequency gravity direction;
- successful moves record a compact motion proxy and no cell may participate in more than one transport exchange in the same pass.

Whole-cell moves/swaps make tracked material mass conservation exact for the ES-006 transport kernel. This is intentionally simpler than a partial-fill fluid solver. Later vertical slices may justify bounded same-material fill relaxation, but they should not replace the shared deterministic transport contract merely for visual convenience.

## Heat — ES-006 baseline

Heat uses a fixed local integer scheme:

- each horizontal/vertical neighbour pair is visited once;
- pairwise exchange is proportional to the smaller material conductivity and is capped per pair;
- equal and opposite pair deltas are accumulated in a fixed 256-element array, so exchange itself is order-independent and energy-balanced before ambient loss;
- each material then moves its temperature toward zero by its centralized ambient-loss amount;
- intermediate arithmetic uses wider integers and final values clamp to `int16_t`.

Units are intentionally game/simulation units. The goals are boundedness, predictable convergence and useful visible thermal gradients, not physical temperature calibration.

## Reactions — ES-006 baseline

Reaction identity is centralized in `kReactionRules`. ES-006 intentionally establishes only the shared primitives required by later hero work:

| Contact | Products | Baseline effect |
| --- | --- | --- |
| lava + water | crust + steam | both products heated |
| sodium_like + water | fire + steam | finite hot fire state + hot gas |
| oil + fire | fire + fire | fuel cell becomes finite hot fire |

Each cell may participate in at most one adjacency reaction during a tick, candidate traversal is deterministic row-major/right/down order, and every applied contact consumes the shared reaction budget. The table is stylized simulation content only.

Fire has a generic finite lifecycle in ES-006: `aux` is a bounded remaining-life counter; when it reaches zero the cell becomes smoke at reduced temperature with mass preserved. This gives later oil/fire and sodium-like scenes a reusable finite energy primitive instead of decorative immortal fire.

## Sodium scene safety semantics

The sodium-like material exists only to generate a recognizable energetic simulation. Do not encode procedures, quantities or experimental guidance for handling real sodium or reactive metals. Implementation is purely visual/game physics.

## Tracer scalar — subsequent milestone

Tracer remains static in ES-006 bulk transport. Tracer/plume work may later model concentration separately or encode concentration in `Cell::aux`; desired future behaviour includes localized injection, advection, diffusion/mixing and concentration-dependent palette mapping.

## Biology — subsequent milestone

Moss/plant and mite-like behavior is not implemented by ES-006. Future biology must remain small, bounded and deterministic and must share the model-owned PRNG.

## Deterministic verification

The ES-004 foundational suite remains unchanged, including its PRNG fixture and exact golden trace. ES-006 adds host-native checks for:

- material-mass conservation under transport and under the current reaction primitives;
- gravity movement along both matrix axes;
- water/oil density ordering and steam rise through liquid;
- distinct centralized liquid mobility/viscosity values;
- bounded pairwise heat exchange and ambient convergence;
- reaction product identity and hard reaction-budget saturation;
- sodium-like/water and oil/fire shared primitives plus finite fire-to-smoke lifecycle;
- separation of disturbance magnitude from gravity direction;
- duplicate `kDynamicsFixture` models producing identical state hashes for the same normalized motion-input sequence;
- reset restoring the initial dynamics state;
- 2,000 randomized dynamics ticks preserving valid bounds, exact tracked mass and budget invariants.

Later hero scenes must add scene-specific fixed-seed traces without weakening these shared mechanics tests.
