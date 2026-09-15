# ESPsand v0 hardware target and characterization

## Current identification state

The owner's boards are identified as ESP32-S3 8×8 RGB-matrix boards with a QMI8658C-class six-axis IMU, BOOT button and USB. Three are available; v0 still targets one board only.

The feature set closely matches Waveshare `ESP32-S3-Matrix` SKU 27119, but the exact owner PCB/revision has **not yet been physically confirmed**. `docs/hardware/BOARD_PROFILE.md` is now the authoritative field-by-field evidence record.

Do not promote likely product-family values to facts merely because public pinouts match.

## Foundation compile profile

ES-001 uses PlatformIO's generic `esp32-s3-devkitc-1` definition as a reproducible **compile profile** with the Arduino framework. It is not the hardware identity.

The ES-001 runtime probe deliberately avoids driving matrix, IMU, BOOT or touch-candidate pins. It reports chip/flash/PSRAM/runtime identity plus the current status of provisional profile values.

## Hardware facts to reconcile

Record, with evidence:

- exact product/revision marking;
- ESP32-S3 module/chip revision, flash and PSRAM if present;
- RGB matrix protocol/type, data GPIO, pixel ordering and colour order;
- QMI8658C I2C SDA/SCL pins, address and interrupt pins;
- IMU physical axis orientation relative to matrix rows/columns;
- BOOT button GPIO and active level;
- exposed free GPIOs;
- which free GPIOs support native ESP32-S3 touch sensing;
- conflicts between touch candidates and matrix/IMU/USB/boot functions;
- practical matrix brightness before board temperature/current becomes undesirable.

## Hardware abstraction boundary

Board drivers must not own simulation semantics. Use narrow interfaces such as:

- `IMatrixOutput`: accept a prepared 8×8 RGB frame plus global luminance/current limit;
- `IImu`: timestamped raw accel/gyro and health/status;
- `IButton`: debounced press-duration events;
- `ITouchZones`: normalized relative disturbances and availability/quality;
- `IClock`: monotonic microsecond/millisecond time;
- `IDiagnostics`: non-blocking serial telemetry.

Host fakes should implement these interfaces or an equivalent seam so orchestration and simulation can test without hardware.

The foundation enforces the first architectural part of this boundary: `lib/espsand_core/` is host-testable C++ with no Arduino/ESP headers, while embedded diagnostics live under `src/board/`.

## IMU requirements

Prefer raw accelerometer + gyro access. Do not rely solely on fused attitude. Useful channels are:

- low-pass acceleration direction -> projected gravity;
- high-pass/transient acceleration -> shake/impulse;
- gyro magnitude and axis rates -> rotation/stirring cues;
- confidence/quality gates when acceleration magnitude departs far from ~1 g.

The matrix-plane orientation transform must be explicit and testable.

## LED constraints

The 64 LEDs can draw substantial current at high simultaneous brightness. v0 therefore requires a single centralized output budget rather than scene-specific arbitrary brightness.

The likely Waveshare-family vendor explicitly warns that excessive brightness can rapidly heat and damage the board. This warning does not provide a numeric safe ceiling. Until measured on the actual board, use a conservative software ceiling and keep its status `NEEDS_PHYSICAL_VALIDATION`.

ES-005 implements that centralized path inside `MatrixOutput`. The current provisional policy is a hard brightness ceiling of **32/255** plus an aggregate frame-load limit of **4096 dimensionless software load units**, where load is derived from `sum(R+G+B) * brightness / 255`. The load figure is not milliamps and is not a measured safe-current or safe-temperature rating.

The second limit intentionally reduces dense output further than sparse highlights; with the current deterministic formula, a 64-pixel full-white frame requested at 255 is reduced to 21/255. Runtime policy may lower/tune the output budget, but `MatrixOutput` clamps its brightness ceiling so code cannot raise physical brightness above the current unvalidated 32/255 hard cap.

Before raising or certifying the hard ceiling, perform sustained physical validation using representative sparse scenes and deliberately dense RGB/white patterns. Prefer an inline USB current meter and temperature probe if available, exercise candidate settings in small increments, soak for 30–60 minutes or longer, and record pattern, ambient conditions, applied limiter state, current/temperature where available, and any resets/USB instability/colour shift. Stop on undesirable heating. Do not infer a sustained safe setting from a short visual check.

## Capacitive experiment constraints

The ESP32-S3 supports native touch sensing on GPIO1–GPIO14 at the silicon level, but exact usable pins depend on board routing.

The desired v0 experiment uses **no added components**. Candidate exposed pads/traces may act as poor-but-useful electrodes. The project values large relative disturbance detection over precise buttons.

Requirements:

- never repurpose a pin required by matrix, IMU, BOOT or USB;
- baseline must adapt slowly to environment drift;
- use change/derivative and common-mode rejection where practical;
- expose raw/normalized values over serial for characterization;
- implement an unavailable/disabled fallback path;
- do not make capacitive success a blocker for the rest of ESPsand.

A future conformal coating may change sensitivity and baseline; touch logic should tolerate recalibration rather than embed absolute factory thresholds.
