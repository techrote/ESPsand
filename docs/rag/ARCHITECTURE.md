# ESPsand v0 firmware architecture

## Toolchain

Accepted foundation: PlatformIO with pinned Espressif32 + Arduino for firmware, plus a pinned native PlatformIO environment for host tests. See `platformio.ini` and `requirements-dev.txt` for exact versions.

The embedded compile profile uses the generic ESP32-S3 DevKitC definition only as a compiler/framework base. `docs/hardware/BOARD_PROFILE.md` is the hardware source of truth for the actual Waveshare ESP32-S3-Matrix target and `platformio.ini` carries the required FH4R2 memory overrides.

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
    touch_zones.*
lib/
  espsand_core/
    include/espsand/
      input/
        touch_normalizer.hpp
      io/
        interfaces.hpp
        touch_types.hpp
        null_touch_zones.hpp
      runtime/
    src/
      touch_normalizer.cpp
test/
  test_foundation/
  test_board_logic/
docs/
  hardware/
  rag/
```

Later material/scene modules should extend this shape without crossing the board/core seam.

## Board-I/O runtime rates

The accepted diagnostic baseline uses independent bounded schedules:

- QMI8658 configured for 1 kHz accel/gyro ODR, polled nominally at 200 Hz;
- matrix presentation target approximately 60 Hz;
- normal serial runtime telemetry 1 Hz;
- BOOT sampled every main-loop iteration through a host-tested debounce/gesture state machine;
- ES-003 touch characterization reads at most one native touch channel every 4 ms, producing a complete seven-channel scan approximately every 28 ms;
- detailed touch telemetry is limited to approximately 10 Hz and only while the touch diagnostic page is active.

Touch channels are pre-initialized during startup because the pinned Arduino-ESP32 `touchRead()` implementation incurs a one-time channel-configuration delay. This prevents first-use touch initialization from creating large stalls inside the steady-state main loop.

Scheduling uses monotonic microsecond time and skips missed periods rather than performing unlimited catch-up work. Later simulation may select 60 or 120 Hz fixed-step model timing after profiling; hardware poll rates must not become simulation semantics.

## Hardware abstraction contracts

The pure core defines narrow interfaces/types for:

- `IClock` — monotonic microseconds;
- `IImu` — timestamped raw + scaled accel/gyro and health/status;
- `IButton` — semantic short/long events;
- `ITouchZones` — optional normalized `cap_a`/`cap_b`/`cap_combo` frames plus diagnostic/status access;
- `IMatrixOutput` — prepared 8×8 RGB frame plus requested global brightness;
- `IDiagnostics` — non-critical telemetry sink.

Host tests implement or exercise these contracts without ESP32 headers. `NullTouchZones` is the permanent clean unavailable implementation; scenes therefore never need to know whether bare-board capacitive sensing succeeded physically.

Board adapters under `src/board/` are the only layer allowed to know concrete GPIOs or Arduino peripheral APIs. `TouchZones` samples candidate pins and delegates baseline/noise/common-mode/hysteresis logic to the pure `TouchNormalizer`/`TouchZoneGate` layer.

`MatrixOutput` is the mandatory physical LED gateway: it owns the NeoPixel object and clamps every frame to the current global development ceiling. Scene code must never instantiate or call the LED driver directly.

## ES-003 touch truth boundary

The official schematic establishes GPIO1–GPIO7 as safe exposed native-touch candidates. Physical sensitivity is a separate question. Until owner-provided characterization demonstrates useful behaviour:

- diagnostic channel data and provisional grouping are available;
- `TouchStatus.hardware_available` may be true when the touch peripheral/candidates initialize;
- `TouchStatus.zones_configured` remains false;
- `TouchFrame.available` remains false;
- no scene can receive a capacitive event accidentally.

If physical evidence supports one/two/combo zones, a later evidence commit can enable the semantic mapping. If not, the abstraction remains unavailable without affecting BOOT + IMU operation.

## Determinism

The future model owns an explicit seeded PRNG. Do not use hidden global randomness inside material or scene rules.

Given seed, scene initial state and a fixed sequence of normalized inputs per tick, host simulation must produce repeatable state hashes/traces. Hardware timing jitter is normalized at the input/orchestration layer before it reaches deterministic tests.

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
- Missing/noisy capacitive input is an unavailable optional capability, not a crash or scene dependency.

## Serial diagnostics

Development output should be machine-readable enough to support capture. Include at minimum:

- firmware/runtime identity;
- current diagnostic/scene mode;
- measured rates and update-time maxima;
- IMU health, detected I2C address and scaled vectors;
- detected button/tap/shake/touch events as those layers exist;
- touch raw/baseline/noise/normalized/common-mode values during ES-003 characterization;
- reaction/event overflow counters once the model exists;
- current global brightness/power limiter state.

Human-readable compact lines are sufficient for v0; a rigid binary protocol is unnecessary.
