# ESPsand v0 implementation roadmap

This roadmap is the reviewed execution sequence. GitHub issues map directly onto these milestones and should normally be completed serially.

## Milestone A — Foundation and hardware truth

### A1. Repository/toolchain/CI + board profile

PlatformIO, native tests, firmware CI, formatting and board-profile documentation.

### A2. Board I/O runtime

Matrix output, QMI8658C adapter, BOOT gesture state machine, monotonic clock, serial diagnostics and hardware abstraction seams.

### A3. No-component capacitive feasibility

ES-003/003A established broad common-mode touch, a pinch-gated coarse slider and explicit bounded external noise. Reliable independent A/B buttons were not supported on the tested bare board.

## Milestone B — Deterministic micro-world

### B1. ES-004 — world/material kernel

Fixed 16x16 world, stable materials, model-owned PCG32, normalized `InputFrame`, lifecycle, bounded work and replay hashes.

### B2. ES-005 — renderer/output budget

Deterministic 2x2 supersampling to 8x8, minority preservation, material/thermal shading and mandatory `MatrixOutput` limiter. Current board policy remains provisional at 32/255 hard brightness plus 4096 software load units.

### B3. ES-006 — shared dynamics

Gravity/buoyancy transport, density/mobility, bounded heat, common reactions, finite fire, motion proxies and work budgets.

## Milestone C — Hero vertical slices

### C1. ES-007 — Lava + Water

First complete product runtime/scene, normalized IMU conditioning, shared material dynamics, product telemetry and physical validation plumbing.

### C2. ES-008 — Sodium-like + Water and Oil + Fire

Adds finite energetic scenes and real multi-scene BOOT cycling while reusing shared reactions/fire lifecycle. Three-scene implementation reached 79/79 native tests before ES-009.

### C3. ES-009 — Moss Garden + Mites and readability baseline

ES-009 adds the slow ecology scene and also incorporates direct physical-board readability feedback from the first chemistry builds.

Implemented baseline:

- Moss Garden with moisture-gated bounded growth energy;
- local moss spread/reinforcement and plant-like anti-gravity shoots;
- fixed three-slot mite agent array with two active initially;
- deterministic seek/wander behavior, biomass feeding, energy gain/loss and starvation;
- slider rain, combo mite/seed event and bounded shake scatter;
- mite/growth state included in model hashing;
- high-contrast post-downsample mite overlay;
- product scene order becomes `Lava -> Sodium -> Oil -> Moss -> Lava`;
- explicit containment wall rings are removed from **all product scenes** because world bounds already block out-of-range transport;
- all logical edge cells and therefore the full 28-LED physical perimeter are available to scene content;
- Lava, Sodium and Oil layouts are re-composed into larger coherent sources/pools/layers/fronts for 8x8 legibility;
- deterministic scene-specific temporal presentation persistence reduces frame-to-frame cell churn without affecting model state;
- reaction and mite overlays remain crisp by being applied after persistence;
- prior chemistry behavior, determinism and output limiting remain covered by regression tests.

The first full ES-009 implementation probe passed **91/91 native tests**, product firmware built at 10.6% RAM / 34.8% flash, and bring-up remained 9.5% / 31.2%.

Physical confirmation is still required for the actual goal behind the rework: scenes should now be more understandable, perimeter LEDs should participate naturally, and persistence should reduce jitter without excessive smear.

## Milestone D — Breadth and showcase polish

### D1. Tracer/Dissolution Plume + one additional scene

Next: implement concentration advection/diffusion and dramatic concentration-dependent colour. Select one additional behavior by novelty-per-complexity after profiling the four current product scenes.

### D2. v0 integration/release

Tune scene order/defaults/transitions, motion thresholds, optional touch mappings, presentation persistence, palette/contrast, serial diagnostics, brightness policy, soak stability, documentation and release metadata. Physical validation evidence from the four current scenes should directly inform this pass.

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
- deterministic fixed-seed model with explicit model-owned randomness;
- host tests for pure logic;
- centralized LED output budget;
- hardware abstraction seams;
- BOOT + IMU sufficiency without touch;
- bounded per-tick work/no unbounded scheduler catch-up;
- tracked-mass conservation for shared transport/reactions except documented scene-owned injection/growth;
- physical display perimeter is content space, not implicit containment UI;
- explicit distinction between automated evidence and physical-board validation.

## Deferred post-v0 directions

Multiple distributed boards, physical topology discovery, ESP-NOW boundary exchange, battery power, enclosure-specific calibration, PC/web visualization, user-authored materials/scenes and richer physical-reference integration remain deferred unless promoted by a future issue.
