# ESPsand v0 implementation roadmap

This roadmap is the reviewed execution sequence. GitHub issues map directly onto these milestones and should normally be completed serially.

## Milestone A — Foundation and hardware truth

### A1. Repository/toolchain/CI + board profile

Established PlatformIO, host-native tests, firmware build CI, formatting and the board characterization harness.

### A2. Board I/O runtime

Established matrix output, QMI8658 raw IMU adapter, BOOT short/long handling, monotonic scheduling, serial diagnostics and host-testable interfaces.

### A3. No-component capacitive feasibility

ES-003/003A established broad `cap_combo`, a pinch-gated coarse slider and an explicit bounded external noise impulse; reliable independent A/B buttons were not supported on the tested bare board.

## Milestone B — Deterministic micro-world

### B1. World/material kernel — ES-004 complete

ES-004 established the host-testable 16×16 deterministic world, compact cell/material registry, model-owned seeded PCG32, fixed-step normalized `InputFrame`, explicit lifecycle seam, bounded event/reaction work counters, stable state hashing and golden replay fixtures.

### B2. Renderer and power-aware output — ES-005 baseline

ES-005 establishes deterministic 2×2 supersampled aggregation to 8×8, centralized material/thermal shading, explicit important-minority preservation, fixed-point exposure/tone mapping, pure diagnostic render modes, and a mandatory physical `MatrixOutput` limiter path.

The current hardware policy uses a provisional 32/255 hard brightness ceiling plus a dimensionless aggregate frame-load envelope of 4096. These are software development limits, not validated current/temperature ratings. Physical soak evidence is still required before raising or certifying them.

### B3. Transport, heat, gas and reaction engine — next

Add gravity-directed transport, density/buoyancy tendency, liquid viscosity differences, bounded momentum proxy if beneficial, gas movement, heat exchange and centralized bounded reactions. Integrate normalized IMU gravity and motion-energy inputs through the ES-004 `InputFrame` contract and render them through the ES-005 output path.

## Milestone C — Hero vertical slices

### C1. Lava + Water

First complete scene proving the stack: liquid contact, cooling/crust, steam/gas, heat glow, tilt, shake/tap and optional capacitive injection.

### C2. Sodium-like + Water and Oil + Fire

Add two visually distinct energetic scenes using the same core. Avoid scene-local physics duplication.

### C3. Moss Garden + Mites

Add slow ecology: moisture-dependent moss/plant growth, 1–3 deterministic mobile mite-like agents, feeding, depletion and recovery.

## Milestone D — Breadth and showcase polish

### D1. Tracer/Dissolution Plume + one additional scene

Implement concentration advection/diffusion and dramatic concentration-dependent colour. Select one extra scene by behaviour-per-complexity after profiling the existing core.

### D2. v0 integration/release

Tune scene order, defaults, transitions, button semantics, motion thresholds, optional touch mappings, serial diagnostics, brightness policy, soak stability, documentation and release metadata.

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
- explicit distinction between automated checks and physical-board validation.

## Deferred post-v0 directions

Do not implement unless explicitly promoted by a future issue: multi-board distributed chunks, ESP-NOW boundary exchange, battery power, enclosure/coating-specific calibration, PC/web visualizer, user-authored materials/scenes, and richer `realref` integration.
