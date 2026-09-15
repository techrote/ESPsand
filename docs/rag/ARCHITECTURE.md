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
  low-pass gravity / shake / tap / spin / capacitive semantics
          ↓
product runtime
  fixed-step scene simulation / lifecycle / timing telemetry
          ↓
pure model + scene policy
  world / materials / deterministic state / bounded work
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

Dependencies point inward. The pure model, shared dynamics, board-independent input state machines, renderer and output limiter contain no Arduino headers, GPIO numbers or LED-driver APIs. ES-004 establishes deterministic state ownership, ES-005 rendering/output budgeting, ES-006 shared material dynamics, and ES-007 the first complete product scene/runtime path.

## Current repository shape

```text
platformio.ini
src/
  main.cpp
  app/
    diagnostic_runtime.*
    scene_runtime.*
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
        motion_interpreter.hpp
      io/
      render/
        output_limiter.hpp
        world_renderer.hpp
      sim/
        dynamics.hpp
        input_frame.hpp
        lava_water_scene.hpp
        materials.hpp
        model.hpp
        prng.hpp
        work_budget.hpp
        world.hpp
    src/
      ...
      dynamics.cpp
      input_frame.cpp
      lava_water_scene.cpp
      model.cpp
      motion_interpreter.cpp
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
  test_lava_water/
docs/
  hardware/
  rag/
```

Later scenes should extend this shape without crossing the board/core seam or duplicating shared mechanics inside scene classes.

## Runtime scheduling — ES-007 product baseline

The normal `esp32s3` firmware now boots `SceneRuntime` directly into `kLavaWater`. `esp32s3_bringup` remains the hardware-isolation/diagnostic target.

The current product runtime uses independent bounded schedules:

- QMI8658 configured for 1 kHz accel/gyro ODR and polled nominally every 5 ms (~200 Hz);
- deterministic model target: one tick every 16,667 us (~60 Hz);
- render target: one frame every 16,667 us (~60 Hz);
- compact product telemetry: 1 Hz;
- BOOT sampled every main-loop iteration;
- touch sampling continues through the accepted ES-003A bounded scan path.

Scheduling uses monotonic microsecond time and **skips missed deadlines rather than running an unbounded catch-up loop**. Hardware poll timing is not simulation state: `Model::step()` receives one sanitized `InputFrame` for each executed logical tick.

The 60 Hz simulation/render rates are provisional implementation targets, not claimed physical performance. `SceneRuntime` now measures `sim_hz`, `render_hz`, `imu_hz`, `max_sim_us` and `max_loop_us` so the first physical run can establish the real budget.

## Normalized IMU interpretation

ES-007 adds pure-core `MotionInterpreter` between raw `ImuSample` and `InputFrame`.

- valid acceleration is sanitized before use;
- a low-pass gravity estimate follows credible near-1g motion slowly;
- transient residual acceleration produces bounded `shake_energy` and `motion_energy` rather than directly steering gravity;
- tap is a bounded one-shot candidate with a 160 ms cooldown;
- gyro Z becomes signed normalized `spin_rate`;
- gravity is projected through the explicit board-to-matrix transform before it reaches the model;
- invalid/missing samples decay disturbance state rather than injecting arbitrary motion.

This keeps raw IMU units, polling jitter and transient acceleration outside deterministic simulation semantics. If IMU has not yet produced a valid sample, the product scene uses a deterministic downward-gravity fallback so autonomous behavior remains visible rather than crashing or freezing.

## Hardware abstraction contracts

The pure core defines narrow interfaces/types for `IClock`, `IImu`, `IButton`, `ITouchZones`, `IMatrixOutput` and `IDiagnostics`. Host tests exercise board-independent policy without ESP32 headers.

Board adapters under `src/board/` are the only layer allowed to know concrete GPIOs or Arduino peripheral APIs. `TouchZones` owns physical sensing/normalization; `SceneRuntime` consumes only its accepted semantic frame.

`MatrixOutput` remains the mandatory physical LED gateway. It owns the NeoPixel driver and every normal product frame still passes through the pure ES-005 `OutputLimiter`. Neither `LavaWaterScene` nor `SceneRuntime` has a second hardware-output path.

## ES-003 / ES-003A touch truth boundary

GPIO1–GPIO7 support deliberately coarse bare-board semantics:

- independent `cap_a` / `cap_b` remain disabled;
- `cap_combo` / `event_combo` represent broad common-mode contact;
- a pinch-gated slider exposes active/position/strength during strong multi-channel contact;
- isolated local excursions may emit explicit bounded `noise_impulse` / `noise_event` values;
- touch noise is external recorded input, never hidden randomness.

ES-007 respects this boundary: slider position can inject bounded lava; combo requests one bounded lava/water contact pulse; disturbance/noise can contribute to crust remixing. The scene does not revive unsupported A/B zones and remains fully usable with BOOT + IMU only.

## ES-004 deterministic model

`World` remains a fixed 16×16 array of 8-byte cells; `Model` owns explicit PCG32 state, lifecycle, event/reaction budgets, deterministic hashing and normalized `InputFrame` consumption. The original `kDeterminismFixture` and its exact golden trace remain unchanged.

`Model::state_hash()` serializes explicit canonical fields rather than object memory. Dynamics/scene schema discriminators are included only for scenes that depend on those contracts, so unrelated foundational traces do not drift merely because a later scene exists.

## ES-005 renderer and output budget

`WorldRenderer` maps each logical 2×2 block to one physical pixel with mass-weighted material shading plus important-minority preservation. `OutputLimiter` remains separate and computes applied brightness from requested brightness, the hard ceiling and a dimensionless aggregate-load envelope.

The current unvalidated physical policy remains a hard 32/255 ceiling plus 4096 software load units. ES-007 currently requests 28/255; the limiter may lower that further for dense frames.

## ES-006 shared dynamics

`DynamicsEngine` remains the sole generic transport/heat/reaction layer. It owns deterministic whole-cell gravity/buoyancy transport, material density/mobility metadata, bounded heat exchange, shared reactions and finite fire lifetime. Scenes may arrange/inject materials and interpret inputs, but they must not reimplement lava/water chemistry or fluid transport.

## ES-007 Lava + Water scene/runtime

`LavaWaterScene` is deterministic scene policy layered around ES-006:

- initializes a large water reservoir, a hot lava body and a seeded contact point;
- periodically injects bounded lava and slower water replenishment so the scene has an autonomous arc;
- maps strong motion to bounded mass-preserving relocation of existing crust cells;
- maps slider and combo touch semantics to bounded material injection only;
- consumes the model-owned PRNG for scene placement/injection choices, so fixed-seed replay remains exact;
- adds `kLavaWaterSceneSchemaVersion` to Lava + Water hash identity.

`SceneRuntime` composes normalized input, executes model ticks, renders beauty output, adds a deterministic sparse highlight to the hottest reaction-derived steam cell, presents through `MatrixOutput`, and emits timing/state/work/output telemetry.

Lifecycle while only one product scene exists:

- short BOOT: exact reset of the current seed;
- long BOOT: “next scene” wraps to Lava + Water with seed+1 and logs that one-scene wrap explicitly;
- ES-008 is expected to replace this temporary wrap with real scene advance.

## Runtime safety

- no blocking `delay()` scene logic;
- no dynamic allocation in hot model/dynamics loops;
- fixed-size world and dynamics scratch storage;
- bounded event/reaction work;
- no recursive reaction processing;
- missed schedules skip rather than backlog indefinitely;
- missing IMU/touch are degraded modes, not crashes;
- every physical LED frame passes through the single output budget.

## Serial diagnostics

The product runtime now reports at ~1 Hz:

- scene/seed/tick/state hash;
- measured IMU/simulation/render rates;
- worst simulation tick and main-loop durations for the reporting window;
- normalized gravity/shake/tap state;
- water/lava/crust/steam masses;
- movement/reaction/fracture/injection counters;
- event/reaction budget use/drop counts;
- renderer minority-preservation count;
- requested/applied LED brightness, estimated load and limiter state.

These fields make physical ES-007 validation copy/pasteable, but actual orientation, visual quality, timing and thermal conclusions require board evidence.
