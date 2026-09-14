# ESPsand v0 firmware architecture

## Baseline toolchain

Planning baseline: PlatformIO with an ESP32-S3-capable Arduino/ESP-IDF environment. The foundation issue may refine this after actual board identification, but must preserve automated host tests and reproducible firmware builds.

## Layering

```text
board drivers
  matrix / imu / button / touch / clock / serial
          ↓
input interpretation
  gravity / shake / tap / spin / cap-zone events
          ↓
runtime orchestrator
  fixed-step simulation / scene lifecycle / diagnostics
          ↓
pure model
  world / materials / transport / heat / gas / reactions / agents
          ↓
renderer
  logical world → 8×8 HDR-ish frame → power-aware output
```

Dependencies should point inward. The pure model must not include Arduino headers, GPIO numbers or LED-driver APIs.

## Suggested repository shape

```text
platformio.ini
src/
  main.cpp
  app/
  board/
  input/
  render/
  scenes/
lib/
  espsand_core/
    world/
    materials/
    reactions/
    biology/
test/
  native/
  fixtures/
docs/
  rag/
```

Exact file names can evolve, but preserve the architecture seam.

## Timing model

Prefer independent rates with bounded non-blocking work:

- IMU acquisition: ~200 Hz or best stable rate supported by driver/board;
- touch acquisition: ~50–100 Hz;
- fixed simulation tick: initially 60 or 120 Hz, benchmarked rather than assumed;
- LED presentation: 60 Hz target;
- serial diagnostics: throttled, never frame-critical.

Use a fixed-step accumulator for simulation. Clamp catch-up work so a transient stall does not create an unbounded spiral.

## Determinism

The model owns an explicit seeded PRNG. Do not use hidden global randomness inside material or scene rules.

Given:

- seed;
- scene initial state;
- fixed sequence of normalized inputs per tick;

host simulation must produce repeatable state hashes/traces.

Hardware timing jitter is normalized at the input/orchestration layer before it reaches deterministic tests.

## Scene interface

A scene should primarily define:

- world initialization;
- enabled materials/reactions;
- palette/render hints if needed;
- mapping of abstract input events to injections or disturbances;
- reset/seed policy;
- optional scene-local scripted events.

A scene should **not** reimplement gravity transport, thermal diffusion, generic combustion or generic reaction scheduling when the core already provides them.

## Runtime safety

- No blocking `delay()`-style scene logic.
- No dynamic allocation in hot per-tick loops unless profiling proves it harmless and bounded.
- Cap particles/agents/events explicitly.
- Ensure reaction chains have per-tick budgets so they cannot recursively explode.
- Watchdog friendliness is a design requirement.
- Diagnostics should expose dropped/capped work rather than silently hiding overload.

## Serial diagnostics

Development output should be machine-readable enough to support capture. Include at minimum:

- firmware version/git identifier if available;
- current scene and seed;
- measured render/simulation rates;
- update-time maxima/averages;
- IMU health and normalized gravity vector;
- detected button/tap/shake/touch events;
- reaction/event overflow counters;
- current global brightness/power limiter state.

Human-readable compact lines are sufficient for v0; a rigid binary protocol is unnecessary.
