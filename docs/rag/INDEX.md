# ESPsand v0 RAG index

This directory is the reusable context pack for autonomous ESPsand v0 implementation.

Read this index first, then the documents required by the active issue. `AGENTS.md` defines the execution contract.

## Documents

- [`PRODUCT.md`](PRODUCT.md) — product goal, non-goals, interaction contract and success criteria.
- [`HARDWARE.md`](HARDWARE.md) — known board capabilities, unknowns that must be measured, and hardware-abstraction rules.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) — firmware module boundaries, scheduling and deterministic-runtime rules.
- [`INPUTS.md`](INPUTS.md) — BOOT button, IMU gesture/gravity model and experimental no-extra-component capacitive zones.
- [`SIMULATION.md`](SIMULATION.md) — internal world representation, materials, transport, thermal/gas/reaction semantics and determinism.
- [`SCENES.md`](SCENES.md) — hero scenes, content priorities and scene-specific inputs.
- [`RENDERING.md`](RENDERING.md) — 16×16-to-8×8 rendering baseline, temporal effects and central brightness/current budget.
- [`TESTING_AND_CI.md`](TESTING_AND_CI.md) — host tests, firmware builds, deterministic golden traces, hardware gates and PR evidence.
- [`PLAN_REVIEW.md`](PLAN_REVIEW.md) — review of the initial concept and changes made before issue publication.
- [`ROADMAP.md`](ROADMAP.md) — improved serial implementation plan and issue dependency map.

## Context capsule

ESPsand v0 targets one ESP32-S3 8×8 RGB-matrix board with a QMI8658C-class 6-axis IMU and one BOOT button. It hangs from USB and presents energetic autonomous material micro-worlds. Short BOOT resets/reseeds the current scene; long BOOT advances scenes. Tilt changes gravity. Shake/tap/swing disturb the world. Broad capacitive zones are desirable if unused native touch-capable GPIOs on the actual board can sense nearby fingers without added components.

The display is intentionally tiny. The simulation should exploit time, internal supersampling, reaction chains, contrast and motion to make 64 LEDs feel richer than their spatial resolution suggests.

Networking, batteries, enclosures and multi-board topology are future work, not v0 requirements.

## Autonomous issue rule

An issue may reference this pack rather than restating every detail, but it must identify the specific required documents and include explicit scope, dependencies, acceptance criteria, validation, PR requirements and merge-after-green-CI instruction.
