# Development and verification

ESPsand v0 uses a pinned PlatformIO/Python toolchain so firmware compilation and host-native tests can be reproduced on Windows, Linux, or macOS.

## Prerequisites

- Python 3.11 is the CI reference interpreter. Newer supported Python versions may also work.
- Git.
- For physical flashing, a USB data cable and the Waveshare ESP32-S3-Matrix target board (SKU 27119).

Create and activate a virtual environment if desired, then install the pinned tools:

```text
python -m pip install -r requirements-dev.txt
```

The repository pins PlatformIO Core `6.1.18`, Espressif32 platform `6.10.0`, native platform `1.2.1`, clang-format `18.1.8`, and Adafruit NeoPixel `1.15.5`.

The firmware compile profile uses PlatformIO's generic `esp32-s3-devkitc-1` definition only as a compiler/Arduino-core base. `platformio.ini` overrides the physical Waveshare ESP32-S3FH4R2 memory profile; `docs/hardware/BOARD_PROFILE.md` is the hardware source of truth.

## One-command verification

Run the same verification sequence as CI:

```text
python tools/ci.py
```

This performs:

1. clang-format check;
2. host-native unit tests;
3. ESP32-S3 firmware compile;
4. ESP32-S3 minimal-bringup compile.

Individual commands include:

```text
python tools/format.py --check
python -m platformio test -e native
python -m platformio run -e esp32s3
python -m platformio run -e esp32s3_bringup
```

To rewrite C/C++ formatting:

```text
python tools/format.py --write
```

The native environment compiles with `-Wall -Wextra -Wpedantic -Werror`.

## Flash the board diagnostic runtime

Build/upload and open the monitor:

```text
python -m platformio run -e esp32s3 -t upload --upload-port COM7
python -m platformio device monitor -e esp32s3 -p COM7 -b 115200
```

Use the actual enumerated port if it is not `COM7`. Native USB may disappear/re-enumerate briefly around reset; PlatformIO can report a transient `ClearCommError` / `PermissionError(13)` before reconnecting.

Current startup reports an `es003-dev` runtime line similar to:

```text
espsand.io start firmware=es003-dev matrix_gpio=14 brightness_ceiling=32 imu_init=1 imu_addr=0x6B who=0x05 revision=0x7C touch_hw=1 touch_channels=7 touch_zones=0
```

`touch_zones=0` is deliberate during ES-003 characterization. Safe candidate routing is known, but semantic capacitive zones stay unavailable until the owner's physical capture demonstrates useful sensitivity.

Once per second the runtime also emits compact IMU/timing telemetry. BOOT events are emitted separately. Missing IMU or unavailable touch sensing is a degraded mode rather than a crash condition.

## BOOT controls

- **short BOOT press:** reset the current diagnostic page/timer;
- **long BOOT press:** advance exactly one diagnostic page while held; release does not generate an additional short event.

Host-tested thresholds are <=600 ms for short and >=800 ms for long, with a deliberate 600–800 ms ambiguity band.

## Diagnostic pages

Pages cycle in this order:

1. **Pixel sweep** — a single amber logical pixel traverses the matrix. Physical calibration established row-major order with USB up, top-left index 0, `index = y * 8 + x`.
2. **Primary colours** — full-panel red, green, blue. Physical calibration established RGB byte order.
3. **Gravity** — a cyan point follows calibrated in-plane gravity using `matrix_x=-imu_y`, `matrix_y=+imu_x`.
4. **Touch characterization** — seven channel bars for GPIO1–GPIO7 plus a common-mode column. This page emits detailed touch telemetry at approximately 10 Hz.

All pages pass through `MatrixOutput`, which clamps output to the development ceiling of 32/255. The touch page requests only 12/255 global brightness.

## ES-003 touch characterization

ES-003 uses only native ESP32-S3 touch sensing and exposed board pads. No foil, wire, resistor, external touch IC or added electrode is part of this experiment.

The official Waveshare schematic establishes GPIO1–GPIO7 as the exposed, otherwise-unused touch-capable set. GPIO10–GPIO14 are reserved by IMU/matrix functions; GPIO19/GPIO20 are USB. Runtime sampling reads one touch channel at a time on a bounded schedule and processes a complete seven-channel scan roughly every 28 ms.

Enter the fourth diagnostic page with three long BOOT presses from cold-boot pixel sweep. Detailed lines have this shape:

```text
touch t_ms=... hw=1 ready=1 zones=0 scans=... cm=... pa=.../0 pb=.../0 pc=.../0 ch1:r... b... d... n... zr... z... a0 ... ch7:...
```

The pure normalizer is host-tested for adaptive baseline tracking, noise normalization, full common-mode cancellation, hysteresis/cooldown and clean unavailable fallback. Product `cap_a`/`cap_b`/`cap_combo` output remains disabled pending physical evidence.

For the exact physical procedure, field definitions and decision criteria, use:

`docs/hardware/TOUCH_CHARACTERIZATION.md`

The important truth boundary is that firmware compilation and host tests can validate the architecture/algorithm, but only board evidence can decide whether one zone, two zones, combo-only sensing, or no useful bare-board sensing is viable.

## Existing physical board calibration

Current accepted observations for the tested unit include:

- 4 MB flash and approximately 2 MB PSRAM;
- RGB matrix on GPIO14;
- RGB colour order;
- linear row-major pixel order in primary orientation (USB up, LEDs facing user);
- QMI8658 SDA/SCL GPIO11/GPIO12, address `0x6B`, WHO_AM_I `0x05`, revision `0x7C`;
- approximately 180 Hz successful IMU polling in captured runs;
- BOOT GPIO0 active-low;
- in-plane gravity transform `x=-raw_y`, `y=+raw_x`;
- Wi-Fi/Bluetooth deliberately kept off for v0 runtime.

Do **not** use short diagnostics as evidence for a safe sustained LED brightness. Thermal/current soak belongs to later power-policy validation.

## Source boundaries

- `lib/espsand_core/` — pure host-testable contracts and input state machines, including touch normalization/gating;
- `src/board/` — hardware-facing ESP32/Arduino adapters, including `TouchZones`;
- `src/app/diagnostic_runtime.*` — non-blocking board diagnostic orchestration;
- `src/main.cpp` — embedded entry point;
- `test/` — host-native tests and fakes;
- `docs/hardware/BOARD_PROFILE.md` — source of truth for hardware facts/evidence status.

Do not promote physical sensitivity, thermal limits or other hardware behaviour from expectation to fact without evidence.
