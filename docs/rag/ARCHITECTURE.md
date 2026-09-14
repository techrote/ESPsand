# ESPsand v0 firmware architecture

## Toolchain

Accepted foundation: PlatformIO with pinned Espressif32 + Arduino for firmware, plus a pinned native PlatformIO environment for host tests. See `platformio.ini` and `requirements-dev.txt` for exact versions.

The embedded compile profile is intentionally generic ESP32-S3; `docs/hardware/BOARD_PROFILE.md` is the hardware source of truth for the actual Waveshare ESP32-S3-Matrix target.

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

Dependencies point inward. The pure model and board-independent input state machines contain no Arduino headers, GPIO numbers or LED-driver APIs.

## Current repository shape

```text
platformio.ini
src/
  main.cpp
  app/
    diagnostic_runtime.*
  board/
    board_profile.hpp
    boot_button.*
    diagnostics.*
    matrix_output.*
    monotonic_clock.hpp
    qmi8658_imu.*
    serial_diagnostics.*
lib/
  espsand_core/
    include/espsand/
      input/
      io/
      runtime/
    src/
test/
  test_foundation/
  test_board_logic/
docs/
  hardware/
  rag/
```

Later material/scene modules should extend this shape without crossing the board/core seam.

## ES-002 runtime rates

The accepted board-I/O diagnostic baseline uses independent bounded schedules:

- QMI8658 configured for 1 kHz accel/gyro ODR, polled at approximately 200 Hz;
- matrix presentation target approximately 60 Hz;
- serial runtime telemetry 1 Hz;
- BOOT sampled every main-loop iteration through a host-tested debounce/gesture state machine.

Scheduling uses monotonic microsecond time and skips missed periods rather than performing unlimited catch-up work.

Later simulation work may select 60 or 120 Hz fixed-step model timing after profiling. Hardware poll rates must not silently become simulation semantics.

## Hardware abstraction contracts

The pure core defines narrow interfaces/types for:

- `IClock` — monotonic microseconds;
- `IImu` — timestamped raw + scaled accel/gyro and health/status;
- `IButton` — semantic short/long events;
- `IMatrixOutput` — prepared 8×8 RGB frame plus requested global brightness;
- `IDiagnostics` — non-critical telemetry sink.

Host tests can implement these interfaces without ESP32 headers. Board adapters under `src/board/` are the only layer allowed to know concrete GPIOs or Arduino peripheral APIs.

`MatrixOutput` is already the mandatory physical LED gateway: it owns the NeoPixel object and clamps every frame to the current global development ceiling. Scene code must never instantiate or call the LED driver directly.

## Determinism

The future model owns an explicit seeded PRNG. Do not use hidden global randomness inside material or scene rules.

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
- Missing IMU is a degraded mode, not a crash condition.

## Serial diagnostics

Development output should be machine-readable enough to support capture. Include at minimum:

- firmware/runtime identity;
- current diagnostic/scene mode;
- measured rates and update-time maxima;
- IMU health, detected I2C address and scaled vectors;
- detected button/tap/shake/touch events as those layers exist;
- reaction/event overflow counters once the model exists;
- current global brightness/power limiter state.

Human-readable compact lines are sufficient for v0; a rigid binary protocol is unnecessary.
