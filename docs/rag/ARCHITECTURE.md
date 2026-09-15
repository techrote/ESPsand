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
  fixed-step scene simulation / catalogue / lifecycle / timing telemetry
          ↓
pure model + scene policy
  world / materials / deterministic state / bounded scene events
          ↓
shared dynamics
  gravity transport / density / gas / heat / bounded reactions / fire lifetime
          ↓
pure renderer
  16×16 logical world → deterministic 8×8 RGB frame
          ↓
physical output gateway
  brightness ceiling + aggregate-load limiter → RGB chain
```

Dependencies point inward. The pure model, scene policies, shared dynamics, board-independent input state machines, renderer and output limiter contain no Arduino headers, GPIO numbers or LED-driver APIs. ES-004 establishes deterministic state ownership, ES-005 rendering/output budgeting, ES-006 shared material dynamics, ES-007 the first complete product scene/runtime path, and ES-008 the first real multi-scene product catalogue.

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
        energetic_scenes.hpp
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
      energetic_scenes.cpp
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
  test_energetic_scenes/
docs/
  hardware/
  rag/
```

Later scenes should extend this shape without crossing the board/core seam or duplicating shared mechanics inside scene classes.

## Runtime scheduling — product baseline

The normal `esp32s3` firmware boots `SceneRuntime` into Lava + Water. `esp32s3_bringup` remains the hardware-isolation/diagnostic target.

The product runtime uses independent bounded schedules:

- QMI8658 configured for 1 kHz accel/gyro ODR and polled nominally every 5 ms (~200 Hz);
- deterministic model target: one tick every 16,667 us (~60 Hz);
- render target: one frame every 16,667 us (~60 Hz);
- compact product telemetry: 1 Hz;
- BOOT sampled every main-loop iteration;
- touch sampling continues through the accepted ES-003A bounded scan path.

Scheduling uses monotonic microsecond time and **skips missed deadlines rather than running an unbounded catch-up loop**. Hardware poll timing is not simulation state: `Model::step()` receives one sanitized `InputFrame` for each executed logical tick.

The target rates are provisional implementation targets, not claimed physical performance. `SceneRuntime` measures `sim_hz`, `render_hz`, `imu_hz`, `max_sim_us` and `max_loop_us` for physical profiling.

## Product scene catalogue — ES-008 baseline

The catalogue is encoded in pure core state rather than as board/runtime conditionals:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Lava + Water
```

`scene_name`, `next_product_scene`, `is_product_scene` and the stable `kProductSceneOrder` keep lifecycle semantics host-testable.

- cold boot begins with Lava + Water;
- short BOOT resets the active model at the same seed;
- long BOOT advances to the next product scene and increments the deterministic seed before initialization;
- fixture scene IDs never enter product cycling.

ES-008 removes the temporary ES-007 one-scene wrap while preserving the same runtime pipeline and fixed-step scheduling.

## Normalized IMU interpretation

Pure-core `MotionInterpreter` sits between raw `ImuSample` and `InputFrame`.

- valid acceleration is sanitized before use;
- a low-pass gravity estimate follows credible near-1g motion slowly;
- transient residual acceleration produces bounded `shake_energy` and `motion_energy` rather than directly steering gravity;
- tap is a bounded one-shot candidate with a 160 ms cooldown;
- gyro Z becomes signed normalized `spin_rate`;
- gravity is projected through the explicit board-to-matrix transform before it reaches the model;
- invalid/missing samples decay disturbance state rather than injecting arbitrary motion.

This keeps raw IMU units, polling jitter and transient acceleration outside deterministic simulation semantics. Before the first valid IMU sample, product scenes receive deterministic downward gravity.

## Hardware abstraction contracts

The pure core defines narrow interfaces/types for `IClock`, `IImu`, `IButton`, `ITouchZones`, `IMatrixOutput` and `IDiagnostics`. Host tests exercise board-independent policy without ESP32 headers.

Board adapters under `src/board/` are the only layer allowed to know concrete GPIOs or Arduino peripheral APIs. `TouchZones` owns physical sensing/normalization; `SceneRuntime` consumes only its accepted semantic frame.

`MatrixOutput` remains the mandatory physical LED gateway. It owns the NeoPixel driver and every product frame passes through the pure ES-005 `OutputLimiter`. No scene policy owns an LED driver or direct hardware-output path.

## ES-003 / ES-003A touch truth boundary

GPIO1–GPIO7 support deliberately coarse bare-board semantics:

- independent `cap_a` / `cap_b` remain disabled;
- `cap_combo` / `event_combo` represent broad common-mode contact;
- a pinch-gated slider exposes active/position/strength during strong multi-channel contact;
- isolated local excursions may emit explicit bounded `noise_impulse` / `noise_event` values;
- touch noise is external recorded input, never hidden randomness.

ES-007 and ES-008 preserve this boundary. Scene policies may assign different meanings to slider/combo, but no scene revives unsupported independent A/B zones. All product scenes remain complete with BOOT + IMU only.

## Deterministic model and hash boundaries

`World` remains a fixed 16×16 array of 8-byte cells; `Model` owns explicit PCG32 state, lifecycle, event/reaction budgets, deterministic hashing and normalized `InputFrame` consumption. The original `kDeterminismFixture` and its exact golden trace remain unchanged.

`Model::state_hash()` serializes explicit canonical fields rather than object memory. Dynamic/product scenes include the shared dynamics schema. Lava + Water, Sodium-like + Water and Oil + Fire additionally include their respective scene schema version and active scene-action stats. ES-008 adds scene IDs 3 and 4 without renumbering older IDs.

## Shared dynamics boundary

`DynamicsEngine` remains the sole generic transport/heat/reaction layer. It owns deterministic whole-cell gravity/buoyancy transport, material density/mobility metadata, bounded heat exchange, shared reactions, bounded reaction impulse and finite fire lifetime.

Scene policies may:

- choose deterministic initial arrangements;
- inject a bounded amount of scene material at explicit scheduled/input events;
- choose one existing material cell for an ignition/start event;
- expose scene-local action counters.

They may **not** create a second fluid solver, reaction propagation loop or flame lifetime system.

This boundary is visible in ES-008:

- Sodium-like + Water inserts/arranges reactants, while common sodium+water reaction/impulse/fire/steam logic supplies the energetic behavior;
- Oil + Fire arranges fuel and may ignite one existing oil cell, while common oil+fire propagation, density transport and fire->smoke lifetime determine the burn.

## Renderer and output budget

`WorldRenderer` maps each logical 2×2 block to one physical pixel with mass-weighted material shading plus important-minority preservation. `OutputLimiter` remains separate and computes applied brightness from requested brightness, the hard ceiling and a dimensionless aggregate-load envelope.

The unvalidated physical policy remains a hard 32/255 ceiling plus 4096 software load units. Product scenes use modest scene-specific requested exposure/brightness and dense frames may be reduced further by the limiter.

`SceneRuntime` may add one deterministic sparse reaction highlight to the hottest actual steam/fire cell on a reaction tick, before output limiting. The highlight therefore cannot bypass the common physical envelope.

## Runtime safety

- no blocking `delay()` scene logic;
- no dynamic allocation in hot model/dynamics loops;
- fixed-size world and dynamics scratch storage;
- bounded event/reaction work;
- no recursive reaction processing;
- missed schedules skip rather than backlog indefinitely;
- missing IMU/touch are degraded modes, not crashes;
- every physical LED frame passes through the single output budget.

## Serial diagnostics — ES-008

Common product telemetry reports:

- scene/seed/tick/state hash;
- measured IMU/simulation/render rates;
- worst simulation tick and main-loop durations for the reporting window;
- normalized gravity/shake/tap state;
- event/reaction budget use/drop counts;
- requested/applied LED brightness, estimated load and limiter state.

Scene-specific lines add:

- Lava + Water: water/lava/crust/steam masses, movement/reaction/fracture/injection counters;
- Sodium-like + Water: water/sodium/fire/steam masses, reaction impulse, movement and injection/burst counters;
- Oil + Fire: water/oil/fire/smoke masses, reactions, expired-fire count, movement and refill/ignition counters.

These fields make physical validation copy/pasteable, but actual visual distinction, handling response, timing and thermal conclusions still require board evidence.
