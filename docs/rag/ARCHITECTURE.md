# ESPsand v0 firmware architecture

## Toolchain

Accepted foundation: PlatformIO with pinned Espressif32 + Arduino for firmware, plus a pinned native PlatformIO environment for host tests. `platformio.ini` carries the corrected ESP32-S3FH4R2 memory profile for the tested Waveshare target.

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
pure renderer
  16×16 world → 8×8 tone-mapped RGB frame
          ↓
physical output gateway
  centralized brightness + aggregate-load limiter → RGB chain
```

Dependencies point inward. Pure model, input interpretation, renderer and limiter code contain no Arduino headers, GPIO numbers or LED-driver APIs.

## Current repository shape

```text
src/
  app/
  board/
    matrix_output.*        # sole NeoPixel owner / physical limiter gateway
lib/espsand_core/
  include/espsand/
    input/
    io/
    runtime/
    sim/
    render/
      world_renderer.hpp
      output_limiter.hpp
  src/
    world_renderer.cpp
    output_limiter.cpp
    ...
test/
  test_simulation_core/
  test_renderer/
  ...
docs/
  hardware/
  rag/
```

## Scheduling

Board-I/O uses bounded independent schedules. The diagnostic baseline polls IMU nominally at 200 Hz, presents the matrix at approximately 60 Hz, reports normal telemetry at 1 Hz, and scans touch channels on the established bounded schedule.

The model still exposes exactly one deterministic `step(InputFrame)` per logical tick; final product simulation frequency is a runtime decision after profiling. Rendering is a pure projection and does not advance model time or consume model PRNG state.

## Hardware abstraction contracts

The core exposes narrow interfaces for clock, IMU, button, touch zones, matrix output and diagnostics. Board adapters under `src/board/` alone know concrete GPIOs/Arduino peripheral APIs.

`MatrixOutput` is the mandatory physical LED gateway. Scene/render code can prepare logical RGB output, but every hardware frame passes through its `OutputLimiter` before brightness is applied. `MatrixOutput` owns the only `Adafruit_NeoPixel` instance.

## ES-004 deterministic model

The model owns a fixed 16×16/256-cell row-major `World`, compact 8-byte `Cell`, centralized stable material IDs, explicit PCG32 state, normalized `InputFrame`, lifecycle methods, bounded work counters and versioned replay hashing.

Same seed + reset/initial state + normalized per-tick inputs must replay identically. External touch/noise irregularity is explicit recorded input rather than hidden randomness.

## ES-005 renderer and output policy

`WorldRenderer` is pure C++17 and deterministically maps each 2×2 logical block to one physical pixel. Beauty mode combines mass-weighted material colour with an importance accent so a small high-priority/emissive phenomenon is not erased by majority coverage. Positive temperature/energy contributes bounded warm emission.

The renderer owns centralized material styles plus integer Q8 exposure and rational tone mapping. It also exposes material-ID, temperature and mass diagnostic projections. Unknown material IDs use a safe fallback instead of indexing outside the registry-aligned style table.

`OutputLimiter` receives the final `Frame8x8` and requested global brightness. It applies the board hard ceiling first, then a deterministic dimensionless aggregate RGB PWM-load envelope. The default load envelope is deliberately provisional and is not a current/thermal safety claim.

The current `MatrixOutput` gateway clamps runtime policy so code cannot raise brightness above the unvalidated 32/255 board development ceiling. Existing diagnostics automatically inherit the same load limiter because they use the same gateway.

## Scene lifecycle seam

Product scenes primarily own deterministic initialization and mapping of normalized inputs to bounded simulation actions. They do not reimplement shared transport/thermal/reaction systems once those exist, and they never write physical LEDs directly.

## Runtime safety

- no blocking scene delays;
- no unbounded frame-critical allocation/work;
- fixed-size deterministic model storage;
- bounded event/reaction chains;
- one centralized physical LED-output budget;
- diagnostics expose capped/dropped work and limiter state as runtime integration matures;
- missing IMU/touch degrade rather than crash.

## Hardware evidence boundary

Automated tests can validate renderer/limiter arithmetic, deterministic output and firmware compilation. They cannot establish a safe sustained LED current/temperature ceiling. Physical soak evidence remains required before raising or certifying the current provisional output limits.
