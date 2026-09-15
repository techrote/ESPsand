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

Subsequent B milestones build on these contracts rather than replacing deterministic state ownership. Intentional model-semantic changes must update the corresponding fixture/schema deliberately.

### B2. Renderer and power-aware output — ES-005 baseline

ES-005 establishes deterministic 2×2 supersampled aggregation to 8×8, important-minority preservation, centralized material/thermal shading, fixed-point exposure/tone mapping, pure render diagnostics, and a mandatory physical `MatrixOutput` limiter path.

The current board policy remains intentionally provisional: 32/255 hard brightness ceiling plus a 4096-unit dimensionless aggregate frame-load envelope. These are development limits, not validated current/temperature ratings; sustained physical soak evidence is still required before raising or certifying them.

### B3. Transport, heat, gas and reaction engine — ES-006 baseline

ES-006 establishes the shared deterministic material-dynamics layer used by later scenes:

- gravity-directed and buoyant whole-cell transport with exact tracked-mass conservation;
- centralized density ordering and distinct water/oil/lava mobility;
- deterministic lateral liquid relaxation and gas dispersion with bounded motion disturbance;
- compact recent-motion proxies rather than a full velocity field;
- fixed-size pairwise thermal exchange plus material ambient loss;
- centralized bounded lava-water, sodium-like-water and oil-fire reaction primitives;
- finite shared fire lifetime;
- hard reaction/event work budgets with no recursive reaction chains;
- a non-product dynamics fixture and randomized conservation/replay stress tests.

Raw IMU data still remains outside the model. ES-006 consumes only the normalized ES-004 `InputFrame` gravity/shake/tap/motion/spin contract and renders later scene state through the ES-005 output path.

## Milestone C — Hero vertical slices

### C1. Lava + Water

First complete scene proving the stack: liquid contact, cooling/crust, steam/gas, heat glow, tilt, shake/tap and optional capacitive injection. This issue is also the first physical timing/scene-loop benchmark for the ES-006 dynamics kernel; shared deficiencies should be fixed in the core rather than hidden in scene code.

### C2. Sodium-like + Water and Oil + Fire

Add two visually distinct energetic scenes using the same core. Avoid scene-local physics duplication.

### C3. Moss Garden + Mites

Add slow ecology: moisture-dependent moss/plant growth, 1–3 deterministic mobile mite-like agents, feeding, depletion and recovery.

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
- bounded per-tick work;
- exact tracked-mass conservation for transport unless an explicitly documented reaction later changes that contract;
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
