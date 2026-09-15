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

### B2. Renderer and power-aware output

Implement supersampled logical-world to 8×8 aggregation, important-minority/emissive preservation, palette/material shading, central brightness/current limiter and render diagnostics.

### B3. Transport, heat, gas and reaction engine

Add gravity-directed transport, density/buoyancy tendency, liquid viscosity differences, bounded momentum proxy if beneficial, gas movement, heat exchange and centralized bounded reactions. Integrate normalized IMU gravity and motion-energy inputs through the ES-004 `InputFrame` contract.

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
