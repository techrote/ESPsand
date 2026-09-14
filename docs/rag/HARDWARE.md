# ESPsand v0 hardware target and characterization

## Known target class

The intended board is an ESP32-S3 module integrating:

- 8×8 addressable RGB LED matrix;
- QMI8658C-class 6-axis accelerometer/gyroscope;
- one BOOT/flash button available for interaction;
- USB power/data;
- exposed GPIOs, some potentially ESP32-S3 native capacitive-touch capable.

Three boards are available to the owner, but v0 targets one board only.

## Do not assume the exact pin map

The product listing/revision must be identified from the actual hardware before pin constants are committed as authoritative. Record, with evidence where possible:

- exact product/revision marking;
- ESP32-S3 module variant, flash and PSRAM if present;
- RGB matrix protocol/type, data GPIO, pixel ordering and colour order;
- QMI8658C I2C SDA/SCL pins and I2C address;
- IMU physical axis orientation relative to matrix rows/columns;
- BOOT button GPIO and active level;
- exposed free GPIOs;
- which free GPIOs support native ESP32-S3 touch sensing;
- conflicts between touch candidates and matrix/IMU/USB/boot functions;
- practical matrix brightness before board temperature/current becomes undesirable.

If the repository later contains a validated board profile, accepted implementation and test evidence supersede this uncertainty list.

## Hardware abstraction boundary

Board drivers must not own simulation semantics. Use narrow interfaces such as:

- `IMatrixOutput`: accept a prepared 8×8 RGB frame plus global luminance/current limit;
- `IImu`: timestamped raw accel/gyro and health/status;
- `IButton`: debounced press-duration events;
- `ITouchZones`: normalized relative disturbances and availability/quality;
- `IClock`: monotonic microsecond/millisecond time;
- `IDiagnostics`: non-blocking serial telemetry.

Host fakes should implement these interfaces or an equivalent seam so orchestration and simulation can test without hardware.

## IMU requirements

Prefer raw accelerometer + gyro access. Do not rely solely on fused attitude. For ESPsand, the useful channels are:

- low-pass acceleration direction -> projected gravity;
- high-pass/transient acceleration -> shake/impulse;
- gyro magnitude and axis rates -> rotation/stirring cues;
- confidence/quality gates when acceleration magnitude departs far from ~1 g.

The matrix-plane orientation transform must be explicit and testable.

## LED constraints

The 64 LEDs can draw substantial current at high simultaneous brightness. v0 therefore requires a single centralized output budget rather than scene-specific arbitrary brightness.

Until measured on the actual board, use a conservative software ceiling. Scene code may request HDR-like logical intensity, but the renderer/output stage must tone-map/clamp to the configured safe envelope.

## Capacitive experiment constraints

The ESP32-S3 supports native touch sensing on a subset of GPIOs, but exact usable pins depend on this board's routing.

The desired v0 experiment uses **no added components**. Candidate exposed pads/traces may act as poor-but-useful electrodes. The project specifically values large relative disturbance detection over precise buttons.

Requirements:

- never repurpose a pin required by matrix, IMU, BOOT or USB;
- baseline must adapt slowly to environment drift;
- use change/derivative and common-mode rejection where practical;
- expose raw/normalized values over serial for characterization;
- implement an unavailable/disabled fallback path;
- do not make capacitive success a blocker for the rest of ESPsand.

A future conformal coating may change sensitivity and baseline; touch logic should tolerate recalibration rather than embed absolute factory thresholds.
