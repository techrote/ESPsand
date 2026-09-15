# ESPsand v0 implementation roadmap

This roadmap is the reviewed execution sequence. GitHub issues map directly onto these milestones and should normally be completed serially, with physical-board feedback allowed to create focused corrective passes between numbered scene milestones.

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

Deterministic 2x2 aggregation to 8x8, minority preservation, material/thermal shading and mandatory `MatrixOutput` limiter.

### B3. ES-006 — shared dynamics

Gravity/buoyancy transport, density/mobility, bounded heat, common reactions, finite fire, motion proxies and work budgets.

## Milestone C — Hero vertical slices

### C1. ES-007 — Lava + Water

First complete product runtime/scene, normalized IMU conditioning, shared material dynamics, product telemetry and physical validation plumbing.

### C2. ES-008 — Sodium-like + Water and Oil + Fire

Adds finite energetic scenes and real multi-scene BOOT cycling while reusing shared reactions/fire lifetime.

### C3. ES-009 — Moss Garden + Mites

Adds slow moisture-dependent ecology and bounded mite-like agents while keeping water transport shared.

## Physical readability/tuning passes after ES-009

Physical board use exposed several display-specific issues that were more important to correct before adding another scene.

### Full-frame/readability correction

Completed before this pass:

- removed redundant product wall rings so the whole perimeter is content space;
- corrected 2x2 coverage projection so one logical sample no longer looks like a full block;
- added deterministic structural contrast and temporal persistence;
- replaced broad slab-heavy chemistry layouts with more structured topologies.

### Sparse-touch-pseudo-HDR baseline — current

Direct physical feedback then showed that even the improved layouts still started/rested with too much material and that the LED chain's useful perceived code range is concentrated near the low end.

Current baseline therefore establishes:

- shared product material population ceiling of **15 logical cells per material**;
- sparse starts for all four scenes;
- slower/cap-aware autonomous replenishment and Moss growth;
- 720-tick host regression checking every material after every tick;
- pinch-slider redefined as direct spatial **secondary-material/actor** control:
  - Lava -> water;
  - Sodium -> water;
  - Oil -> local ignition/fire;
  - Moss -> mite;
- direct slider actions occur after the current physics pass so the touched state is visible near selected X before the next physics tick;
- Moss begins with one autonomous mite instead of pre-populating more agents;
- Beauty ordinary output recentered into **0..127**, visually centred around ~63;
- **128..255** reserved for sparse pseudo-HDR hot/reactive/agent state;
- product scalar request/ceiling permit 255 so that code-domain distinction can reach the LEDs;
- the existing **4096-unit aggregate frame-load limiter remains mandatory** and is the automatic dense-frame governor;
- no electrical/thermal certification is inferred from that software policy.

This tuning pass must be physically reviewed before using its perceptual thresholds as permanent release constants.

## Milestone D — Breadth and showcase polish

### D1. Tracer/Dissolution Plume + one additional scene

Next planned breadth milestone after physical validation of the sparse/HDR baseline: implement concentration advection/diffusion and dramatic concentration-dependent colour. New scenes should begin sparse and build complexity through state/input rather than pre-filling large material regions.

### D2. v0 integration/release

Tune scene order/defaults/transitions, motion thresholds, optional touch mappings, palette/contrast, persistence, pseudo-HDR thresholds, aggregate-load policy, soak stability, serial diagnostics and release metadata.

Release tuning should be driven by actual board evidence collected from the current four scenes, particularly:

- perceived ordinary centre/max;
- pseudo-HDR contrast usefulness;
- touch-location correspondence;
- sustained current/temperature behavior;
- material population/readability balance.

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
physical readability/tuning
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
- one centralized physical LED output path;
- hardware abstraction seams;
- BOOT + IMU sufficiency without touch;
- bounded per-tick work/no unbounded scheduler catch-up;
- tracked-mass conservation for shared transport/reactions except documented scene-owned creation;
- full physical perimeter remains content space;
- current product scenes stay inside the sparse material-population contract;
- ordinary/pseudo-HDR brightness semantics remain presentation-only and cannot affect model state;
- explicit distinction between automated evidence and physical-board validation.

## Deferred post-v0 directions

Multiple distributed boards, physical topology discovery, ESP-NOW boundary exchange, battery power, enclosure-specific calibration, PC/web visualization, user-authored materials/scenes and richer physical-reference integration remain deferred unless promoted by a future issue.
