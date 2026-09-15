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
  gravity / shake / tap / spin / capacitive semantics
          ↓
runtime orchestrator
  fixed-step simulation / scene lifecycle / diagnostics
          ↓
pure model
  world / materials / deterministic state / bounded work seams
          ↓
renderer
  logical world → 8×8 HDR-ish frame → power-aware output
```

Dependencies point inward. The pure model and board-independent input state machines contain no Arduino headers, GPIO numbers or LED-driver APIs. ES-004 establishes the pure deterministic substrate; transport, heat, reactions, agents and rendering remain later milestones.

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
      core/
      input/
      io/
      runtime/
      sim/
        input_frame.hpp
        materials.hpp
        model.hpp
        prng.hpp
        work_budget.hpp
        world.hpp
    src/
      ...
      input_frame.cpp
      model.cpp
      prng.cpp
      world.cpp
test/
  test_foundation/
  test_board_logic/
  test_touch_semantics/
  test_simulation_core/
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

Scheduling uses monotonic microsecond time and skips missed periods rather than performing unlimited catch-up work. ES-004 deliberately does not choose the final simulation frequency. The model exposes one deterministic `step(InputFrame)` per logical tick; the runtime will choose and schedule a fixed rate after profiling. Hardware poll rates must not become simulation semantics.

## Hardware abstraction contracts

The pure core defines narrow interfaces/types for:

- `IClock` — monotonic microseconds;
- `IImu` — timestamped raw + scaled accel/gyro and health/status;
- `IButton` — semantic short/long events;
- `ITouchZones` — optional normalized capacitive frame plus diagnostic/status access;
- `IMatrixOutput` — prepared 8×8 RGB frame plus requested global brightness;
- `IDiagnostics` — non-critical telemetry sink.

Host tests implement or exercise these contracts without ESP32 headers. `NullTouchZones` is the permanent clean unavailable implementation; scenes therefore never need to know whether bare-board capacitive sensing succeeded physically.

Board adapters under `src/board/` are the only layer allowed to know concrete GPIOs or Arduino peripheral APIs. `TouchZones` samples candidate pins and delegates baseline/noise/common-mode/hysteresis logic to the pure touch-normalization layer.

`MatrixOutput` is the mandatory physical LED gateway: it owns the NeoPixel object and clamps every frame to the current global development ceiling. Scene code must never instantiate or call the LED driver directly.

## ES-003 / ES-003A touch truth boundary

GPIO1–GPIO7 are validated safe exposed native-touch candidates on the tested board. Physical characterization established a deliberately coarse product semantic:

- `cap_a` and `cap_b` remain disabled;
- `cap_combo` and its gated event represent broad common-mode edge contact;
- a pinch-gated coarse slider exports active/position/strength only during strong multi-channel contact;
- isolated fast local excursions may export bounded `noise_impulse` / `noise_event` values;
- the noise signal is explicit external input, never model seed material or hidden randomness.

Missing or noisy capacitive sensing still degrades cleanly; BOOT + IMU remain sufficient.

## ES-004 deterministic model

The model substrate lives under `lib/espsand_core/` and is ordinary C++17 with no Arduino/ESP dependency.

- `World` is a fixed 16×16, 256-cell, row-major array.
- `Cell` is a fixed 8-byte value containing material ID, mass/fill, signed motion proxies, temperature/energy, material-specific auxiliary state and flags.
- material identity and category metadata are centralized in one versioned registry with stable numeric IDs;
- `Model` owns a PCG32 PRNG. Its seed, state and stream increment are explicit deterministic state; core logic must not use `rand()`, timestamps or platform entropy;
- `InputFrame` is the normalized, hardware-independent input for one logical tick;
- `Model::init`, `reset`, `reseed` and `step` provide the lifecycle seam used by later scenes/runtime work;
- event and reaction `WorkBudget` instances provide fixed, saturating per-tick accounting seams;
- the only ES-004 scene is a tiny deterministic fixture used to prove PRNG/state transitions. It is not a product hero scene.

The same seed, reset/initial state and `InputFrame` sequence must replay identically. External touch/noise irregularity affects a run only through recorded `InputFrame` values and does not perturb PRNG state unless future simulation code explicitly chooses to consume the PRNG for a defined rule.

### State hash contract

`Model::state_hash()` is a stable FNV-1a 64-bit regression/replay identity over explicit, canonical little-endian fields. It covers the hash/material/cell schema versions, dimensions, scene/configuration, seed, tick, PRNG state, fixture state, budget counters and every cell in deterministic row-major order.

The hash deliberately does **not** serialize raw struct memory, so padding and host ABI do not define replay identity. It is not a cryptographic integrity or security mechanism. Intentional semantic changes may change the hash and must update the golden trace in the same change with an explanation.

## Scene lifecycle seam

A later product scene should primarily define:

- deterministic world initialization from model configuration/seed;
- enabled materials/reactions;
- mapping of normalized `InputFrame` fields/events to bounded simulation actions;
- reset/reseed policy;
- optional scene-local scripted events;
- palette/render hints outside the pure material rules where needed.

BOOT-derived `InputFrame::boot_event` carries semantic lifecycle intent, but ES-004 does not make the generic model silently reset itself from inside `step()`. The runtime/scene controller owns reset/reseed/scene-advance policy and calls the explicit lifecycle methods.

A scene should **not** reimplement gravity transport, thermal diffusion, generic combustion or generic reaction scheduling once later core milestones provide those systems.

## Runtime safety

- No blocking `delay()`-style scene logic.
- No dynamic allocation in hot per-tick loops unless profiling proves it harmless and bounded.
- Frame-critical world/model storage is fixed-size in ES-004.
- Cap particles/agents/events explicitly.
- Reaction chains must participate in bounded per-tick work rather than recurse without limit.
- Watchdog friendliness is a design requirement.
- Diagnostics should expose dropped/capped work rather than silently hiding overload.
- Missing IMU is a degraded mode, not a crash condition.
- Missing/noisy capacitive input is an unavailable optional capability, not a crash or scene dependency.

## Serial diagnostics

Development output should be machine-readable enough to support capture. Include as systems become active:

- firmware/runtime identity;
- current diagnostic/scene mode;
- measured rates and update-time maxima;
- IMU health, detected I2C address and scaled vectors;
- detected button/tap/shake/touch events;
- touch raw/baseline/noise/normalized/common-mode values during characterization;
- model seed/state identity and reaction/event overflow counters once the runtime executes model scenes;
- current global brightness/power limiter state.

Human-readable compact lines are sufficient for v0; a rigid binary protocol is unnecessary.
