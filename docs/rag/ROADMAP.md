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

Adds finite energetic scenes and real multi-scene BOOT cycling while reusing shared reactions/fire lifetime.

### C3. ES-009 — Moss Garden + Mites and first readability baseline

ES-009 adds moisture-dependent ecology, bounded mite agents, full-perimeter product worlds and render-only temporal persistence. It also removes redundant product wall rings and first re-composes the chemistry scenes for larger 8x8-readable shapes.

The ES-009 baseline reached **91/91 native tests**, product firmware at 10.6% RAM / 34.8% flash, and bring-up at 9.5% / 31.2%.

### C3.1. Issue #35 — structured-diversity readability correction

Direct physical feedback after ES-009 showed the first re-composition had moved too far toward large homogeneous blocks. Issue #35 therefore refines the presentation contract before adding more scene breadth:

- Beauty projection now preserves how many of the four logical subcells actually occupy each physical LED, preventing a one-cell stream from appearing as full as a four-cell block;
- water/oil/lava/moss receive mild deterministic structural contrast based on stable coordinates, material boundaries and actual motion—not PRNG/twinkle noise;
- Lava uses an irregular water shoreline, varied fill depth, thin meandering hot stream and falling refill rivulets;
- Sodium uses an uneven pool, separated single drops and falling water rivulets;
- Oil uses shallow irregular water, discontinuous fuel ribbons/pockets and a sparse left-originating ignition front;
- Moss model state is intentionally unchanged; the renderer exposes structure in its large wet/green bodies;
- the no-wall-ring/full-28-LED-perimeter contract is preserved;
- dedicated anti-blockiness tests guard initial and settled product projections without replacing causal scene tests.

The implementation probe reached **95/95 native tests**, including 4/4 new readability tests, with product firmware at 10.6% RAM / 34.9% flash and bring-up unchanged at 9.5% / 31.2%.

Subjective success still requires the physical board: the goal is coherent material bodies with visible internal structure, not either broad flat slabs or random confetti.

## Milestone D — Breadth and showcase polish

### D1. Tracer/Dissolution Plume + one additional scene

Next after physical review of the corrected four-scene baseline: implement concentration advection/diffusion and dramatic concentration-dependent colour. Apply the structured-diversity lesson from #35 from the start. Select one additional behavior by novelty-per-complexity after profiling the existing scenes.

### D2. v0 integration/release

Tune scene order/defaults/transitions, motion thresholds, optional touch mappings, presentation persistence, coverage/structural contrast, palette, serial diagnostics, brightness policy, soak stability, documentation and release metadata. Physical validation evidence should directly inform this pass.

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
C3.1
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
- renderer/presentation state never feeds back into deterministic simulation;
- readability favors structured information density over both monolithic slabs and arbitrary decorative noise;
- explicit distinction between automated evidence and physical-board validation.

## Deferred post-v0 directions

Multiple distributed boards, physical topology discovery, ESP-NOW boundary exchange, battery power, enclosure-specific calibration, PC/web visualization, user-authored materials/scenes and richer physical-reference integration remain deferred unless promoted by a future issue.
