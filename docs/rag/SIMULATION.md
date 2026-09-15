# ESPsand v0 simulation model

## Internal resolution — ES-004 baseline

The deterministic world is **16×16 logical cells** with a fixed capacity of 256 cells in row-major order. The later renderer will aggregate this world to the physical 8×8 LEDs, preserving the intentional four-logical-samples-per-LED supersampling baseline.

ES-004 does not implement that renderer and does not lower the model to 8×8. A future resolution change is a model-semantic change and must preserve bounded storage, deterministic host tests and an acceptable firmware frame budget.

## Cell/state model

The common `Cell` is an 8-byte trivially copyable value:

```text
MaterialId material   // uint8 stable registry ID
uint8      mass       // fill/mass proxy
int8       motion_x   // compact signed momentum proxy
int8       motion_y
int16      temperature // temperature/energy proxy in model-defined units
uint8      aux        // material-specific compact state
uint8      flags
```

These are substrate fields, not a claim that final transport/thermal semantics already exist. `aux` is reserved for bounded material-specific state such as concentration, fuel, biomass or age. Later systems should prefer this compact representation unless profiling/tests justify a change.

World access is bounds-checked through `try_cell` / `set_cell`; iteration and hashing use deterministic row-major order. Material accounting reports cell counts, per-material mass, occupied cells, total mass and invalid-state diagnostics.

## Material registry

Material IDs are centralized and versioned. ES-004 locks the initial identity set:

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

Material names are expressive game/simulation names, not claims of chemical fidelity. Behaviour belongs to later centralized transport/reaction systems rather than scene-local material identity conditionals.

Mite-like mobile agents remain a likely separate bounded structure rather than a dense material ID.

## PRNG and deterministic state

`Model` owns an explicit PCG32 generator. The default stream selector is 54 (stored internally as odd increment 109); seeding follows the standard two-step PCG initialization. PRNG state and increment are exposed for deterministic inspection and included in state hashing.

No core material/scene rule may draw from hidden/global entropy, `rand()`, timestamps or hardware noise. The same seed, initial/reset state and normalized per-tick input sequence must produce the same state trace.

The PRNG sequence itself is fixture-locked in host tests. Changing PRNG algorithm, seeding semantics or draw ordering is therefore an explicit model-semantic change.

## Fixed-step model and lifecycle

`ModelConfig` currently contains the explicit seed, scene ID and per-tick event/reaction budget limits. `Model` provides:

- `init(config)` — install configuration and initialize deterministic state;
- `reset()` — restore tick zero and the configured seed/initial scene state;
- `reseed(seed)` — replace the configured seed and reset deterministically;
- `step(InputFrame)` — execute exactly one logical simulation tick;
- state inspection, invariant checks and a deterministic state hash.

ES-004 does not choose a wall-clock update frequency. Hardware/runtime code is responsible for scheduling a fixed number of logical ticks and for constructing normalized `InputFrame` values before crossing the pure-model boundary.

The sole ES-004 `kDeterminismFixture` scene places a fixed border, one water cell and one seeded tracer marker. A sufficiently strong explicit tap relocates that marker using the model PRNG; capacitive/noise fixture fields demonstrate recorded external state changes without consuming PRNG. This fixture exists only to lock deterministic mechanics and is not a hero scene or transport implementation.

## Bounded per-tick work

`WorkBudget` supplies fixed-width event/reaction counters with atomic all-or-nothing consumption. When a requested unit count exceeds the remaining budget, no partial work is accepted and the dropped counter saturates predictably. The model resets these counters each tick.

ES-004 uses the event budget in the deterministic fixture and reserves the reaction budget as the seam later reaction work must consume. This prevents later chain effects from requiring a new unbounded scheduling model.

## State hashing and trace fixture

State hashing uses versioned FNV-1a 64 over explicitly serialized little-endian fields rather than object memory. It covers all state represented by ES-004 that can affect replay/future evolution, including seed/configuration, tick, PRNG state, fixture state, work counters and every cell.

It is a **regression/replay identity**, not a cryptographic integrity guarantee. The hash schema is versioned. Intentional changes to model semantics must deliberately update the golden trace fixture.

The locked ES-004 trace uses seed `0x0123456789ABCDEF`, event budget 2 and reaction budget 3:

```text
initial      0x4943A6C732CA020D
after tick 1 0x75A4B3C9249EF546
after tick 2 0xFC14CC3D7A9C7408
after tick 3 0xB411D621F3D3F1C6
after tick 4 0x8F0F30D22E87FB14
```

The last tick intentionally requests three fixture events against a two-event budget and locks the resulting `used=2`, `dropped=1` behavior.

## Transport — subsequent milestone

Transport is not implemented by ES-004. The later kernel should prioritize recognizable macroscopic behaviour at tiny scale:

- gravity-directed movement;
- density ordering / buoyancy tendencies;
- lateral relaxation for liquids;
- viscosity differences;
- bounded momentum/advection proxy where it materially improves slosh;
- gas rise/dispersion;
- solids that settle and block.

Conservation should be testable for material mass except where reactions explicitly create/remove/transform it.

## Heat — subsequent milestone

Heat diffusion is not implemented by ES-004. Later work should use a cheap bounded thermal model with local exchange, material-specific rates, thresholds and ambient loss. Numerical stability matters more than physical units.

## Reactions — subsequent milestone

Reaction behavior is not implemented by ES-004. Later work should use centralized/data-driven rules rather than scene-specific cross-material conditionals, and every reaction/effect must participate in the bounded per-tick reaction/event accounting seam.

Illustrative future game rules include lava + water cooling/steam, hot material + oil combustion, sodium-like + water energy/gas, and water-assisted biomass growth. These are simulation effects, not real-world handling guidance.

## Sodium scene safety semantics

The sodium-like material exists only to generate a recognizable energetic simulation. Do not encode procedures, quantities or experimental guidance for handling real sodium or reactive metals. Implementation is purely visual/game physics.

## Tracer scalar — subsequent milestone

Tracer/plume scenes may later model a concentration field separate from bulk water or encode concentration in cell `aux`. Desired future behaviour includes localized injection, advection, diffusion/mixing and dramatic concentration-dependent palette mapping.

## Biology — subsequent milestone

Moss/plant and mite-like behavior is not implemented by ES-004. Future biology must remain small, bounded and deterministic and must share the model-owned PRNG.

## Deterministic verification

The ES-004 host suite locks:

- fixed seed + same inputs -> identical per-tick state hashes across repeated models;
- exact PRNG output fixture;
- exact multi-tick golden hash trace;
- deterministic reset and reseed behavior;
- different seeds causing controlled stochastic fixture divergence;
- bounds-safe world access and fixed 16×16/256 capacity;
- material cell/mass accounting and invalid-state detection;
- predictable event/reaction budget saturation semantics;
- external capacitive/noise inputs leaving PRNG state unchanged unless a programmed rule explicitly consumes it;
- malformed/non-finite input sanitization;
- 5,000-tick randomized replay/stress invariants and randomized out-of-bounds probes.

Later transport, heat, reactions, rendering and biology must add their own conservation/boundedness fixtures without weakening these baseline tests.
