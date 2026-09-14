# ESPsand

ESPsand v0 is a tiny, physically manipulated material-world showcase for an ESP32-S3 board with an 8×8 RGB matrix, QMI8658C-class 6-axis IMU, USB power and one BOOT button.

The goal is **not** to prove that an ESP32 can run a falling-sand toy. The goal is to make sixty-four LEDs feel like a surprisingly energetic world: lava hitting water, sodium-like reactive material fizzing in water, oil flashing into flame, tracer plumes changing colour, moss spreading, plants growing, and tiny mites wandering and nibbling.

The board should be compelling while dangling from USB with no enclosure and no external control surface.

## Core interaction contract

- **Short BOOT press:** reset/reseed the current scene.
- **Long BOOT press:** advance to the next scene.
- **Tilt:** continuously changes the gravity vector projected into the simulated world.
- **Shake / swing / tap:** injects kinetic disturbance or triggers scene-specific responses.
- **Experimental component-free capacitive sensing:** broad, deliberately imprecise interaction zones using otherwise-unused ESP32-S3 touch-capable GPIOs if the actual board revision permits it.
- **USB:** power first; serial diagnostics and development second. No phone app, network service, or PC runtime is required for the showcase.

## Development status

**ES-001 foundation:** the repository has a pinned PlatformIO toolchain, host-native test seam, ESP32-S3 compile target, formatting/CI harness, safe serial hardware probe, and explicit board-profile evidence model.

No material simulation or hero scene is implemented in the foundation milestone.

## Build and test

See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) for complete setup, verification, flashing and serial-probe instructions.

After installing the pinned Python tools:

```text
python -m pip install -r requirements-dev.txt
python tools/ci.py
```

`tools/ci.py` runs the same three checks as GitHub Actions: clang-format verification, host-native tests, and the ESP32-S3 firmware compile.

Hardware assumptions and the exact evidence still required are tracked in [`docs/hardware/BOARD_PROFILE.md`](docs/hardware/BOARD_PROFILE.md).

## v0 product principles

1. Prefer **apparent life and material interaction** over raw simulation breadth.
2. Keep the pure simulation deterministic and host-testable even when showcase runs are seeded randomly.
3. Do not hard-code uncertain board pin assumptions as facts. Identify and record the actual board revision and evidence status.
4. Treat the 8×8 LEDs as a display, not necessarily as the simulation grid. A higher-resolution internal field may be downsampled to 8×8.
5. Build energetic-looking effects under a conservative global brightness/current budget; visual punch should come from contrast, timing and motion, not sustained maximum LED power.
6. Physical sodium, combustible oils, live organisms, dyes, UV sources and similar materials are **not** required by ESPsand. All hazardous-looking scenes are simulations.
7. Networking and multi-board topology are explicitly deferred from v0.

## RAG planning pack

Implementation agents should begin with [`docs/rag/INDEX.md`](docs/rag/INDEX.md).

The pack contains the authoritative v0 product intent, hardware assumptions, architecture, input model, simulation semantics, scene catalogue, test/CI expectations, reviewed roadmap, and execution contract for autonomous issue work.

## Planned implementation sequence

The GitHub issues are intended to be completed serially unless an issue explicitly states otherwise:

1. repository/toolchain and hardware-characterisation foundation;
2. board I/O abstraction, matrix, IMU, BOOT button and diagnostics;
3. component-free capacitive interaction feasibility work;
4. deterministic host-testable world/material kernel;
5. rendering/downsampling and LED power budget;
6. motion-aware material transport, thermal/gas behaviour and reactions;
7. polished hero scenes;
8. biology, tracer-plume and showcase integration;
9. release-quality polish and hardware validation.
