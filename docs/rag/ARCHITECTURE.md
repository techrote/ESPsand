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
shared dynamics
  gravity transport / density / gas / heat / bounded reactions
          ↓
pure renderer
  16×16 logical world → deterministic 8×8 RGB frame
          ↓
physical output gateway
  brightness ceiling + aggregate-load limiter → RGB chain
```

Dependencies point inward. The pure model, shared dynamics, board-independent input state machines, renderer and output limiter contain no Arduino headers, GPIO numbers or LED-driver APIs. ES-004 establishes deterministic state ownership, ES-005 establishes rendering/output budgeting, and ES-006 establishes the shared material-dynamics layer used by later hero scenes.

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
      render/
        output_limiter.hpp
        world_renderer.hpp
      runtime/
      sim/
        dynamics.hpp
        input_frame.hpp
        materials.hpp
        model.hpp
        prng.hpp
        work_budget.hpp
        world.hpp
    src/
      ...
      dynamics.cpp
      input_frame.cpp
      model.cpp
      output_limiter.cpp
      prng.cpp
      world.cpp
      world_renderer.cpp
test/
  test_foundation/
  test_board_logic/
  test_touch_semantics/
  test_simulation_core/
  test_renderer/
  test_dynamics/
docs/
  hardware/
  rag/
```

Later material/scene modules should extend this shape without crossing the board/core seam or duplicating shared mechanics inside scene classes.

## Board-I/O runtime rates

The accepted diagnostic baseline uses independent bounded schedules:

- QMI8658 configured for 1 kHz accel/gyro ODR, polled nominally at 200 Hz;
- matrix presentation target approximately 60 Hz;
- normal serial runtime telemetry 1 Hz;
- BOOT sampled every main-loop iteration through a host-tested debounce/gesture state machine;
- ES-003 touch characterization reads at most one native touch channel every 4 ms, producing a complete seven-channel scan approximately every 28 ms;
- detailed touch telemetry is limited to approximately 10 Hz and only while the touch diagnostic page is active.

Touch channels are pre-initialized during startup because the pinned Arduino-ESP32 `touchRead()` implementation incurs a one-time channel-configuration delay. This prevents first-use touch initialization from creating large stalls inside the steady-state main loop.

Scheduling uses monotonic microsecond time and skips missed periods rather than performing unlimited catch-up work. The model exposes one deterministic `step(InputFrame)` per logical tick. Hardware poll rates must not become simulation semantics.

Rendering is a pure projection and does not advance the model, consume model PRNG state or read wall-clock entropy. ES-006 dynamics similarly receive only normalized `InputFrame` values; raw accelerometer/gyro samples and hardware sampling jitter remain outside the model boundary.

The current diagnostic firmware does not yet execute a product scene continuously, so ES-006 cannot honestly claim a physical simulation-tick timing result. The required later on-board benchmark is explicit: run the first vertical-slice scene with fixed simulation cadence, report `sim_hz`, worst/rolling `max_tick_us`, render cadence and dropped/capped work in the 1 Hz serial telemetry, then exercise gravity/shake and reactions for at least several minutes. A tick budget must be selected from measured data rather than inferred from host tests.

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

`MatrixOutput` is the mandatory physical LED gateway: it owns the NeoPixel object and every frame passes through its pure `OutputLimiter` before hardware brightness is applied. The limiter enforces both the current global development ceiling and a provisional aggregate RGB PWM-load envelope. Scene code must never instantiate or call the LED driver directly.

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
- event and reaction `WorkBudget` instances provide fixed, saturating per-tick accounting seams.

The original `kDeterminismFixture` remains unchanged as a foundational PRNG/input/replay fixture. The same seed, reset/initial state and `InputFrame` sequence must replay identically. External touch/noise irregularity affects a run only through recorded `InputFrame` values and does not perturb PRNG state unless simulation code explicitly chooses to consume the PRNG for a defined rule.

### State hash contract

`Model::state_hash()` is a stable FNV-1a 64-bit regression/replay identity over explicit, canonical little-endian fields. It covers the hash/material/cell schema versions, dimensions, scene/configuration, seed, tick, PRNG state, fixture state, budget counters and every cell in deterministic row-major order.

The hash deliberately does **not** serialize raw struct memory, so padding and host ABI do not define replay identity. It is not a cryptographic integrity or security mechanism. ES-006 adds a dynamics-schema discriminator only for `kDynamicsFixture`; the ES-004 fixture retains its existing exact hashes.

## ES-005 deterministic renderer and output budget

`WorldRenderer` maps each fixed 2×2 logical block to one physical pixel. Beauty rendering combines mass-weighted material colour with a deterministic high-importance accent so small fire/lava/steam/tracer/biomass features are not erased merely because another material occupies most of the block.

Material shading is centralized. Positive `Cell::temperature` contributes bounded warm logical emission; tracer/moss `aux` values provide bounded palette modulation. Integer Q8 exposure and rational tone mapping produce an 8-bit frame without hidden random dithering or floating-point replay dependencies.

The renderer also exposes deterministic material-ID, temperature and mass diagnostic projections. Invalid material IDs use a safe fallback and are counted rather than indexing outside the style table.

`OutputLimiter` is deliberately separate from simulation and material shading. It computes the applied global brightness from the requested value, the hard ceiling and a dimensionless aggregate RGB PWM-load envelope. Its default load value is a conservative software policy, not a milliamps/temperature claim.

## ES-006 shared dynamics

`DynamicsEngine` is a stateless pure-core service operating on `World`, one normalized `InputFrame`, an explicit tick index and the model-owned work budgets.

- movement uses fixed scans and fixed-size claim arrays; one cell cannot participate in two transport exchanges during a tick;
- density, mobility, gas/solid classification, thermal conductivity and ambient loss live in one centralized material-dynamics table;
- diagonal normalized gravity is converted to a deterministic cardinal duty sequence rather than continuous floating-point positions;
- shake/motion/tap/spin increase bounded disturbance/mobility while low-frequency gravity direction remains separate;
- pairwise heat exchange accumulates into a fixed 256-element delta array before application;
- three centralized adjacency reaction primitives cover the shared mechanics required by later lava/water, sodium-like/water and oil/fire scenes;
- reactions consume `WorkBudget` units, never recurse, and optional local reaction impulses consume event budget;
- finite fire lifetime is generic shared material behavior, not scene animation.

`kDynamicsFixture` exists only to test this shared layer under the full `Model::step()`/hash lifecycle. It is not a hero scene.

## Scene lifecycle seam

A product scene should primarily define:

- deterministic world initialization from model configuration/seed;
- enabled material injection and scene-local scripted events;
- mapping of normalized `InputFrame` fields/events to bounded scene actions;
- reset/reseed policy;
- optional palette/render hints outside the shared material rules.

BOOT-derived `InputFrame::boot_event` carries semantic lifecycle intent, but the generic model does not silently reset itself from inside `step()`. The runtime/scene controller owns reset/reseed/scene-advance policy and calls the explicit lifecycle methods.

Scenes must use the ES-006 shared transport, thermal and reaction mechanisms. If a later vertical slice exposes a genuine deficiency, fix the shared layer and add a regression test rather than hiding bespoke physics in scene code.

## Runtime safety

- No blocking `delay()`-style scene logic.
- No dynamic allocation in hot per-tick loops unless profiling proves it harmless and bounded.
- Frame-critical world/model/dynamics scratch storage is fixed-size.
- Cap particles/agents/events explicitly.
- Reaction chains participate in bounded per-tick work and never recurse.
- Watchdog friendliness is a design requirement.
- Diagnostics should expose dropped/capped work rather than silently hiding overload.
- Missing IMU is a degraded mode, not a crash condition.
- Missing/noisy capacitive input is an unavailable optional capability, not a crash or scene dependency.
- Every physical LED frame passes through the single centralized ES-005 output budget.

## Serial diagnostics

Development output should be machine-readable enough to support capture. Include as systems become active:

- firmware/runtime identity;
- current diagnostic/scene mode;
- measured rates and update-time maxima;
- IMU health, detected I2C address and scaled vectors;
- normalized gravity/confidence and disturbance energy once scene runtime is active;
- detected button/tap/shake/touch events;
- touch raw/baseline/noise/normalized/common-mode values during characterization;
- model seed/state identity and reaction/event overflow counters;
- transport/reaction counts and simulation tick maxima during physical profiling;
- current global brightness/power limiter state.

Human-readable compact lines are sufficient for v0; a rigid binary protocol is unnecessary.
