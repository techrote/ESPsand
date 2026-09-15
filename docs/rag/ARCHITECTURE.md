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
pure rendering + presentation
  16x16 world -> 8x8 frame -> bounded temporal persistence / overlays
          ↓
physical output gateway
  brightness ceiling + aggregate-load limiter -> RGB chain
```

Dependencies point inward. Pure model, scene policy, shared dynamics, input conditioning, renderer/presentation helpers and output limiter contain no Arduino GPIO/LED-driver dependencies.

Milestone ownership:

- ES-004: deterministic world/state/PRNG contracts;
- ES-005: renderer and centralized output budgeting;
- ES-006: shared material dynamics;
- ES-007: first product scene/runtime;
- ES-008: multi-scene energetic catalogue;
- ES-009: bounded ecology/agents plus physical-readability rework of all existing product scenes.

## Current repository shape

The pure core now additionally contains:

```text
include/espsand/render/scene_effects.hpp
include/espsand/sim/moss_garden_scene.hpp
src/scene_effects.cpp
src/moss_garden_scene.cpp
test/test_moss_garden/
```

`MossGardenScene` remains inside pure core. Mites are bounded model agents, not board/runtime objects.

## Product world boundaries — ES-009

Product scenes use the fixed 16x16 array bounds as containment. `DynamicsEngine::try_move()` already rejects targets outside `World::in_bounds()`, so the explicit one-logical-cell wall ring previously placed by product scene initializers was redundant.

ES-009 removes that wall ring from Lava + Water, Sodium-like + Water and Oil + Fire, and Moss Garden never adds one. This is intentionally different from the non-product deterministic/dynamics fixtures, which may retain explicit wall cells because their layouts are regression substrates.

Consequences:

- all 256 logical positions are available to product content;
- no product-scene wall material is reserved solely for containment;
- because 2x2 logical blocks map to one physical pixel, the full 8x8 perimeter—28 LEDs—can now display scene content instead of being influenced by a hidden border;
- explicit wall material remains available for future authored obstacles.

## Product scene catalogue — ES-009

The pure-core order is:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

`scene_name`, `next_product_scene`, `is_product_scene` and `kProductSceneOrder` keep lifecycle semantics host-testable.

- cold boot: Lava + Water;
- short BOOT: exact current-seed reset;
- long BOOT: next scene + deterministic seed increment;
- fixture IDs never enter product cycling.

## Runtime scheduling

Normal `esp32s3` boots `SceneRuntime`; `esp32s3_bringup` remains the hardware-isolation target.

Current bounded schedules remain approximately:

- IMU poll: 5 ms (~200 Hz target);
- model tick: 16,667 us (~60 Hz target);
- render: 16,667 us (~60 Hz target);
- telemetry: 1 Hz.

Missed deadlines are skipped rather than creating unbounded catch-up. These rates remain provisional until physical telemetry is captured.

## Input boundary

Pure-core `MotionInterpreter` keeps low-pass gravity separate from transient shake/motion/tap/spin and projects through the explicit board-to-matrix transform. `TouchZones` owns hardware sensing; runtime consumes only accepted semantic touch fields.

Independent A/B touch zones remain disabled. Slider/combo/noise semantics are the only current bare-board touch contract, and all product scenes remain functional with BOOT + IMU alone.

## Deterministic model and Moss Garden agents

`World` remains 16x16 fixed storage of 8-byte cells. `Model` owns PCG32, tick, work budgets, scene objects and state hashing.

Moss Garden adds a fixed three-slot mite array plus a bounded growth-energy reservoir. Two mites start active. A mite has only x, y, energy and active state. Those values and growth energy are hashed because they affect future simulation. No agent container grows dynamically.

Moss biomass remains ordinary `MaterialId::kMoss` world state. Mites occupy an overlay state and therefore may consume a moss cell without replacing it with an agent material.

Scene schema identities:

- Lava + Water: version 2 after ES-009 composition rework;
- Sodium-like + Water: version 2;
- Oil + Fire: version 2;
- Moss Garden: version 1.

The original ES-004 fixture hash path remains unchanged.

## Shared dynamics boundary

`DynamicsEngine` remains the sole generic transport/heat/reaction layer. It owns material mobility/density, gravity/buoyancy transport, heat exchange, common reactions, bounded reaction impulse and finite fire lifetime.

Scene policy may arrange/inject bounded material, choose an ignition/start event, manage bounded ecology state, and expose counters. It may not duplicate fluid transport, chemistry propagation or fire lifetime.

Moss Garden specifically reuses shared water transport. Plant growth observes nearby water and projected gravity, while mite logic is a separate bounded ecology process.

## Rendering and presentation boundary — ES-009

`WorldRenderer` remains the pure 16x16 -> 8x8 material projection. `scene_effects` adds two deterministic presentation operations:

1. **temporal persistence**: blend the current base RGB frame with the immediately previous base frame using a scene-specific fixed integer weight;
2. **mite overlay**: project active agent coordinates to their 8x8 positions with a high-contrast marker.

Presentation history is runtime/render state, not simulation state. It never affects `Model`, PRNG, reactions, movement or state hashes. Scene reset/change clears history.

Render order is deliberate:

```text
WorldRenderer
 -> temporal persistence
 -> save smoothed base frame
 -> reaction highlight
 -> mite overlay (Moss Garden)
 -> MatrixOutput
 -> OutputLimiter
 -> LEDs
```

Reaction flashes and mites are therefore not smeared into history and every effect remains constrained by the common physical output gateway.

## Readability rework of existing scenes

ES-009 changes composition rather than replacing physics:

- Lava + Water: central lava source over a full-width lower basin;
- Sodium-like + Water: paired logical reactant drops over a full-width pool;
- Oil + Fire: broad oil layer above water with a left-originating ignition front.

These macro layouts are intended to make orientation and causal evolution easier to read on 64 LEDs. CI proves deterministic distinction and prior behavioral contracts still pass; only a physical board run can prove subjective readability improved.

## Renderer/output budget

The central physical policy is unchanged: hard brightness ceiling 32/255 plus 4096 dimensionless aggregate load units. Scene-specific requested brightness/exposure stays below that ceiling and dense output may be reduced further.

No scene, presentation helper or agent overlay owns an LED driver or bypasses `MatrixOutput`/`OutputLimiter`.

## Runtime safety

- no blocking scene delays;
- no dynamic allocation in hot model/dynamics/ecology loops;
- fixed world, fixed agent array and fixed scratch storage;
- bounded event/reaction work;
- no recursive reaction processing;
- missed schedules skip rather than backlog;
- missing IMU/touch degrades cleanly;
- every physical frame passes through one limiter.

## Serial diagnostics

Common telemetry retains scene/seed/tick/hash, measured rates, worst tick/loop times, gravity/motion and limiter state.

Scene lines report existing chemistry counters plus Moss Garden fields: water/moss mass, active mite count, growth energy, growth/reinforcement, mite movement/feed/starvation, rain/seed/scatter and event-budget usage.

Telemetry provides evidence plumbing; visual readability, handling response and thermal safety still require physical-board observations.
