# ESPsand v0 implementation roadmap

This roadmap is the reviewed execution sequence. GitHub issues map directly onto these milestones and should normally be completed serially.

## Milestone A — Foundation and hardware truth

### A1. Repository/toolchain/CI + board profile

Establish PlatformIO, host-native tests, firmware build CI, formatting, and a documented board profile/characterization harness. Unknown physical values remain explicitly marked until measured.

### A2. Board I/O runtime

Implement matrix output, QMI8658C raw IMU driver/adapter, BOOT short/long state machine, monotonic clock, serial diagnostics, fake hardware interfaces and a simple diagnostic showcase.

### A3. No-component capacitive feasibility

Enumerate safe candidate touch GPIOs from the validated board profile, implement diagnostic sampling/baselines/common-mode normalization, characterize the bare-board signal and expose only physically supported bounded semantics with graceful fallback.

ES-003/003A established broad `cap_combo`, a pinch-gated coarse slider and an explicit bounded external noise impulse; reliable independent A/B buttons were not supported on the tested bare board.

## Milestone B — Deterministic micro-world

### B1. World/material kernel — ES-004 baseline

ES-004 establishes the host-testable 16×16 deterministic world, compact cell/material registry, model-owned seeded PCG32, fixed-step normalized `InputFrame`, explicit lifecycle seam, bounded event/reaction work counters, stable state hashing and golden replay fixtures.

### B2. Renderer and power-aware output — ES-005 baseline

ES-005 establishes deterministic 2×2 supersampled aggregation to 8×8, important-minority preservation, centralized material/thermal shading, fixed-point exposure/tone mapping, pure render diagnostics, and a mandatory physical `MatrixOutput` limiter path.

The current board policy remains intentionally provisional: 32/255 hard brightness ceiling plus a 4096-unit dimensionless aggregate frame-load envelope. These are development limits, not validated current/temperature ratings.

### B3. Transport, heat, gas and reaction engine — ES-006 baseline

ES-006 establishes shared deterministic gravity/buoyancy transport, centralized density/mobility, bounded thermal exchange, common reaction primitives, finite fire lifetime, compact motion proxies and hard work budgets. Raw IMU data remains outside the model; shared dynamics consumes only normalized `InputFrame` semantics.

## Milestone C — Hero vertical slices

### C1. Lava + Water — ES-007 baseline

ES-007 establishes the first complete product scene and the normal product runtime:

- normal firmware boots directly into deterministic Lava + Water;
- a strong water reservoir + hot lava initial layout guarantees useful early interaction;
- autonomous bounded lava/water replenishment keeps the scene active;
- all flow, heat, gas and lava-water conversion reuse ES-006 shared mechanics;
- cooled crust is persistent and visibly darker; steam remains a high-priority pale/bright feature;
- pure `MotionInterpreter` converts raw IMU samples into low-pass matrix gravity plus separate shake/tap/motion/spin signals;
- tilt changes flow; strong motion can perform bounded mass-preserving crust remixing;
- accepted ES-003A slider/combo touch semantics provide optional lava/contact injection without reviving rejected A/B zones;
- normal runtime schedules provisional ~200 Hz IMU sampling, ~60 Hz simulation and ~60 Hz rendering with no catch-up backlog;
- 1 Hz telemetry exposes scene state, rates, tick maxima, work budgets and output-limiter decisions;
- `test_lava_water` locks deterministic scene initialization, reaction evolution, tilt divergence, reset, fixed-seed replay, input conditioning and randomized bounded replay.

The first CI integration probe passed 66/66 native tests and built both firmware targets. Physical orientation, visible quality, actual rates and thermal/brightness behavior still require the documented board checklist; these are not inferred from CI.

### C2. Sodium-like + Water and Oil + Fire — ES-008 baseline

ES-008 adds two product scenes without duplicating the shared physics stack:

- Sodium-like + Water uses the centralized sodium-like/water reaction, generic reaction impulse, shared particle/liquid transport, finite fire and buoyant steam/smoke. Scene policy supplies only deterministic setup, sparse reactant/water replenishment and bounded optional slider/combo injection.
- Oil + Fire uses centralized oil density/mobility, the common oil/fire reaction and generic fire-to-smoke lifetime. Its autonomous cycle deliberately includes fuel consumption and an extinction interval before sparse refill and re-ignition.
- the stable product catalogue is now `Lava + Water -> Sodium-like + Water -> Oil + Fire -> Lava + Water`;
- long BOOT advances through that catalogue; short BOOT resets the current seeded scene;
- rejected independent A/B touch zones remain disabled. Slider/combo are the only optional product touch semantics, and all three scenes remain demonstrable with BOOT + IMU alone;
- scene-specific schema IDs/stats extend deterministic state hashing without changing older scene IDs;
- runtime rendering still uses `WorldRenderer` plus the single `MatrixOutput` limiter, with sparse reaction highlighting generalized to actual hot steam/fire cells;
- ES-008 native tests lock finite consumption/extinction, reaction impulses, scene order, fixed-seed replay, rendering distinction and randomized bounded replay for both new scenes.

Physical visual distinction, real handling response, measured scene timing and sustained output comfort remain explicit board-validation work rather than CI claims.

### C3. Moss Garden + Mites

Next: add slow ecology with moisture-dependent moss/plant growth, 1–3 deterministic mobile mite-like agents, feeding, depletion and recovery. It should extend scene variety rather than modifying the already-shared energetic mechanics.

## Milestone D — Breadth and showcase polish

### D1. Tracer/Dissolution Plume + one additional scene

Implement concentration advection/diffusion and dramatic concentration-dependent colour. Select one extra scene by behaviour-per-complexity after profiling the existing core.

### D2. v0 integration/release

Tune scene order, defaults, transitions, button semantics, motion thresholds, optional touch mappings, serial diagnostics, brightness policy, soak stability, documentation and release metadata. Capture explicit hardware-validation checklist/results where available.

## Dependency graph

```text
A1
 ↓
A2
 ├────→ A3
 ↓
B1
 ↓
B2
 ↓
B3
 ↓
C1
 ↓
C2
 ↓
C3
 ↓
D1
 ↓
D2
```

A3 may proceed after A2 and does not block B1 if touch remains unavailable.

## Cross-cutting invariants

Every milestone preserves:

- single-board v0 scope;
- no mandatory network/cloud/app runtime;
- deterministic fixed-seed core with explicit model-owned randomness;
- host tests for pure logic;
- centralized LED output budget;
- hardware abstraction seams;
- button + IMU sufficiency even without touch;
- bounded per-tick work and no unbounded scheduler catch-up;
- exact tracked-mass conservation for shared transport/reactions, except intentional documented scene material injection;
- explicit distinction between automated checks and physical-board validation.

## Deferred post-v0 directions

Document but do not implement unless explicitly promoted by a future issue:

- multiple boards as distributed simulation chunks;
- physical edge sensing/topology discovery;
- ESP-NOW boundary exchange;
- battery power;
- enclosures/coating-specific calibration;
- PC/web visualizer;
- user-authored materials/scenes;
- richer physical-reference integration with `realref`.
