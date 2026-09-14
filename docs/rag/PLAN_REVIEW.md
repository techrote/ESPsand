# ESPsand v0 plan review

This document records the review performed before publishing implementation issues. The purpose is to prevent the issue set from merely restating the first idea without addressing sequencing, hidden dependencies and hardware uncertainty.

## Initial concept strengths

The initial concept had several strong properties:

- one-board scope is physically simple and immediately demoable;
- the 8×8 matrix forces emphasis on temporal behaviour rather than visual detail;
- the IMU naturally turns handling into a continuous control surface;
- one BOOT button is enough for reset/next-scene semantics;
- intentionally imprecise capacitive interaction suits the playful material-world concept;
- energetic chemistry-like scenes and slow biology provide complementary timescales;
- a 16×16 internal world can provide richer movement without making the firmware large.

## Problems identified in the first-pass plan

### 1. Too much hardware knowledge was being treated as known

The exact matrix GPIO/order, QMI8658C pins/address/orientation, BOOT GPIO, flash/PSRAM and free touch-capable pins depend on the exact board revision. A plan that starts by implementing scenes risks baking in incorrect pin assumptions.

**Improvement:** make hardware characterization and a board profile the first implementation gate.

### 2. Capacitive touch could become a schedule trap

The no-extra-components touch idea is attractive, but exposed-pad geometry may simply be poor. If scenes depend on it, the project can stall before the core showcase exists.

**Improvement:** isolate capacitive sensing behind an optional interface, give it a dedicated experimental issue, and require all scenes to remain usable through button + IMU.

### 3. Scene-first development would duplicate physics

Implementing lava, oil, tracer and plants independently would encourage bespoke scene code that cannot be tested or reused.

**Improvement:** build a small material/transport/thermal/reaction kernel before hero scenes, while avoiding a giant general-purpose engine.

### 4. A 16×16 world alone does not guarantee a richer 8×8 image

Naive downsampling can erase exactly the small reaction fronts and agents that supersampling was intended to preserve.

**Improvement:** make renderer aggregation/power policy its own milestone with mixed-material tests before scene polish.

### 5. Energetic visuals could accidentally become a power/thermal problem

A scene author might use full-white frames for every reaction.

**Improvement:** centralize logical-HDR-to-output mapping and brightness/current limiting so scenes cannot bypass it.

### 6. Motion input needed separation into gravity and disturbance

Using raw accelerometer direction directly as gravity during shakes would make the world point in arbitrary directions exactly when it is being handled most aggressively.

**Improvement:** low-pass gravity estimate plus separate high-frequency shake/tap energy and gyro-derived spin.

### 7. The original scene list was too broad for the first implementation loop

Eight scenes before architecture stabilization creates a large polishing surface.

**Improvement:** implement four hero scenes first: lava/water, sodium-like/water, oil/fire, moss/mites. Add tracer plume and one extra scene only after those demonstrate the shared kernel.

### 8. Hardware-only testing needed explicit truth boundaries

Remote agents can compile and host-test but cannot legitimately claim a physical matrix, IMU or touch pad worked.

**Improvement:** split automated acceptance from explicit hardware gates and require agents to leave precise manual validation procedures rather than inventing evidence.

### 9. Multi-board thinking could contaminate v0 architecture

The distributed-tile concept is excellent future work, but preparing every subsystem for arbitrary network topology now would slow the single-board showcase.

**Improvement:** declare networking/multi-board topology out of scope for v0. Preserve clean module boundaries, but add no networking abstraction without a current need.

## Improved plan

The improved sequence is therefore:

1. toolchain + CI + hardware characterization/profile;
2. board I/O foundation and serial diagnostics;
3. optional capacitive feasibility experiment;
4. deterministic world/material core;
5. supersampled renderer + centralized brightness/current policy;
6. transport/heat/gas/reaction framework driven by IMU gravity/motion;
7. lava/water hero scene to validate the complete vertical slice;
8. sodium-like/water + oil/fire, reusing the same framework;
9. biology/mites;
10. tracer plume + one additional scene;
11. integration, tuning, soak testing, documentation and v0 release.

The vertical-slice decision is deliberate: lava/water is implemented before the other energetic scenes because it exercises liquid transport, temperature, phase-like transformation, gas and rendering together. If that scene requires architectural contortions, fix the core before multiplying content.

## Exit criterion for planning phase

Planning is complete when:

- the RAG pack exists and is internally consistent;
- issues follow the improved sequence;
- every issue can be picked up autonomously with explicit dependencies, required reading, acceptance criteria, validation and PR/merge rules;
- no issue silently assumes successful capacitive sensing or unknown board pin assignments.
