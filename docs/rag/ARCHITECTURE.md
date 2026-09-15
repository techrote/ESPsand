# ESPsand v0 firmware architecture

## Toolchain

Accepted foundation: PlatformIO with pinned Espressif32 + Arduino for firmware, plus a pinned native PlatformIO environment for host tests. `docs/hardware/BOARD_PROFILE.md` remains the source of truth for the Waveshare ESP32-S3-Matrix hardware; the generic ESP32-S3 DevKitC definition is only the framework/compiler base.

## Layering

```text
board drivers
  matrix / imu / button / touch / clock / serial
          ↓
input interpretation
  low-pass gravity / shake / tap / spin / capacitive semantics
          ↓
product runtime
  fixed-step scene simulation / catalogue / lifecycle / timing telemetry
          ↓
pure model + scene policy
  world / materials / deterministic state / bounded scene events / fixed agents
          ↓
shared dynamics
  gravity transport / density / gas / heat / bounded reactions / fire lifetime
          ↓
pure renderer
  16x16 world -> coverage-aware / structurally shaded 8x8 Beauty frame
          ↓
runtime presentation
  bounded temporal persistence -> transient reaction/agent overlays
          ↓
physical output gateway
  brightness ceiling + aggregate-load limiter -> RGB chain
```

Dependencies point inward. Model, scene policy, shared dynamics, input conditioning, renderer/presentation helpers and output limiter contain no Arduino GPIO/LED-driver dependencies.

Milestone ownership:

- ES-004: deterministic world/state/PRNG contracts;
- ES-005: renderer and centralized output budgeting;
- ES-006: shared material dynamics;
- ES-007: first product scene/runtime;
- ES-008: multi-scene energetic catalogue;
- ES-009: bounded ecology/agents, full-perimeter product worlds and temporal presentation persistence;
- issue #35: coverage-aware projection, deterministic structural contrast and structured-diversity scene topology.

## Product world boundaries

Product scenes use the fixed 16x16 array bounds as containment. `DynamicsEngine` rejects out-of-bounds moves, so no product scene reserves a containment wall ring.

Consequences:

- all 256 logical positions are available to product content;
- the full 8x8 physical perimeter (28 LEDs) can display scene content;
- explicit wall material remains available for authored obstacles and non-product fixtures.

## Product scene catalogue

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

Cold boot begins with Lava. Short BOOT resets the active scene at the same seed. Long BOOT advances one scene and increments the deterministic seed. Fixture scenes never enter product cycling.

## Runtime scheduling

Normal `esp32s3` boots `SceneRuntime`; `esp32s3_bringup` remains the hardware-isolation target.

Current bounded targets remain:

- IMU poll: 5 ms (~200 Hz);
- model tick: 16,667 us (~60 Hz);
- render: 16,667 us (~60 Hz);
- telemetry: 1 Hz.

Missed deadlines are skipped instead of accumulated. Physical telemetry remains authoritative for actual timing.

## Deterministic model and scene schemas

`World` remains fixed 16x16 storage of compact cells. `Model` owns PCG32, tick, work budgets, product scene policy and future-affecting scene state.

Moss Garden owns a fixed three-slot mite array plus bounded growth energy. Mite x/y/energy/active values are hashed because they affect future ecology.

Current product schemas:

- Lava + Water: **3**;
- Sodium-like + Water: **3**;
- Oil + Fire: **3**;
- Moss Garden: **1**.

Issue #35 changes chemistry starting/source topology and therefore deliberately increments only the three chemistry schemas. Moss model semantics do not change; its readability improvement is renderer-only. The original ES-004 fixture hash path remains unchanged.

## Shared dynamics boundary

`DynamicsEngine` remains the sole generic transport/heat/reaction layer. It owns density/mobility, gravity/buoyancy transport, heat exchange, centralized reactions, bounded reaction impulse and finite fire lifetime.

Scene policy may arrange/inject bounded material, choose a bounded ignition/start event, manage bounded ecology state and expose counters. It may not duplicate fluid transport, chemistry propagation or fire lifetime.

Issue #35 changes only deterministic layout/source geometry for Lava/Sodium/Oil; shared mechanics remain authoritative.

## Pure renderer boundary — issue #35

`WorldRenderer` still performs the pure 16x16 -> 8x8 projection and cannot mutate simulation state.

Beauty mode now has two additional deterministic responsibilities required by physical readability:

1. **logical coverage preservation** — after mass-weighted colour aggregation, the number of occupied logical subcells in each physical 2x2 block scales base colour with the fixed Q8 table `0, 136, 176, 216, 256`. A one-cell stream therefore remains visible but no longer looks as full as a four-cell block;
2. **structural material contrast** — water/oil/lava/moss receive a small deterministic scale derived from fixed output coordinate/material phase, same-material physical neighbors and actual cell motion proxies.

Neither operation consumes PRNG or clock state. A fixed world yields a fixed frame. Material-ID, temperature and mass diagnostic modes retain their own semantics.

Minority/high-priority accent blending occurs after coverage and structural scaling, preserving sharp fire/steam/reaction readability.

This is distinct from runtime temporal persistence: coverage and structural shading are functions of the **current world**, while persistence is previous-frame presentation history.

## Runtime presentation boundary

`scene_effects` remains responsible for:

- temporal persistence: blend current base RGB with the previous base frame using scene-specific fixed weights;
- mite overlay: project fixed model-agent coordinates to high-contrast 8x8 markers.

Presentation history is not model state and is reset on scene reset/change.

Render order is:

```text
WorldRenderer
  [coverage + structural shading + minority accents]
 -> temporal persistence
 -> save smoothed base frame
 -> reaction highlight
 -> mite overlay (Moss Garden)
 -> MatrixOutput
 -> OutputLimiter
 -> LEDs
```

Transient overlays therefore stay crisp and every effect remains constrained by the common physical gateway.

## Structured-diversity chemistry layouts — issue #35

Physical feedback showed that ES-009's broad coherent shapes still contained too many featureless same-material blocks. The chemistry scenes now use:

- **Lava:** irregular water shoreline/depth, thin meandering lava stream and falling edge rivulets;
- **Sodium:** irregular pool, separated single drops and falling water rivulets;
- **Oil:** shallow irregular water plus discontinuous oil ribbons/pockets and sparse left-originating fire.

Moss Garden model topology stays unchanged; the shared coverage/structural renderer exposes variation in its wet/green regions without inserting decorative simulation noise.

The policy is intentionally between two bad extremes: neither broad rectangular slabs nor random confetti.

## Input boundary

`MotionInterpreter` keeps low-pass gravity separate from transient shake/motion/tap/spin. `TouchZones` owns hardware sensing. Independent A/B touch zones remain disabled; slider/combo/noise are the accepted bare-board semantics. All scenes remain functional with BOOT + IMU alone.

## Output budget

The central physical policy is unchanged: hard brightness ceiling 32/255 plus 4096 dimensionless aggregate load units. Scene-specific brightness/exposure requests remain below the ceiling and dense output may be reduced further.

No scene, renderer helper, persistence pass or overlay owns an LED driver or bypasses `MatrixOutput`/`OutputLimiter`.

## Runtime safety

- no blocking scene delays;
- no dynamic allocation in hot model/dynamics/ecology/render loops;
- fixed world, fixed agents and fixed renderer scratch arrays;
- bounded event/reaction work;
- no recursive reaction processing;
- missed schedules skip rather than backlog;
- missing IMU/touch degrades cleanly;
- every physical frame passes through one limiter.

## Serial diagnostics

Common telemetry retains scene/seed/tick/hash, measured rates, worst tick/loop times, gravity/motion and limiter state. Scene lines expose chemistry or ecology counters.

Telemetry provides evidence plumbing. Subjective readability, handling response and thermal safety still require physical-board observations.
