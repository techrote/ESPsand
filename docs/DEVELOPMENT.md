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

The repository pins:

- PlatformIO Core `6.1.18`;
- PlatformIO Espressif32 platform `6.10.0`;
- PlatformIO native platform `1.2.1`;
- clang-format `18.1.8`;
- Adafruit NeoPixel `1.15.5` for the embedded matrix adapter.

The firmware compile profile still uses PlatformIO's generic `esp32-s3-devkitc-1` definition. The physical target is the Waveshare ESP32-S3-Matrix; the generic profile is only the reproducible compiler/Arduino-core target and the firmware does not rely on DevKitC-specific peripherals.

## One-command verification

Run the same verification sequence as CI:

```text
python tools/ci.py
```

This performs, in order:

1. clang-format check;
2. host-native unit tests;
3. ESP32-S3 firmware compile.

Individual commands are:

```text
python tools/format.py --check
python -m platformio test -e native
python -m platformio run -e esp32s3
```

To rewrite C/C++ formatting:

```text
python tools/format.py --write
```

The native environment compiles with `-Wall -Wextra -Wpedantic -Werror`.

## Flash the ES-002 board-I/O diagnostic runtime

Build and upload:

```text
python -m platformio run -e esp32s3 -t upload
```

Then open the serial monitor:

```text
python -m platformio device monitor -b 115200
```

The firmware is non-blocking. It drives only the explicitly documented ES-002 resources: the matrix, provisional GPIO0 BOOT input, and provisional GPIO11/GPIO12 QMI8658 I2C bus. It does not use IMU interrupt pins or candidate touch pins.

### Serial output

Startup emits the foundation probe and an ES-002 line similar to:

```text
espsand.io start firmware=es002-dev matrix_gpio=14 brightness_ceiling=32 imu_init=1 imu_addr=0x6B who=0x05 revision=0x..
```

Once per second it emits compact runtime telemetry:

```text
runtime mode=gravity imu_ok=1 imu_addr=0x6B imu_rate_hz=... failures=0 acc_g=(...,...,...) gyro_dps=(...,...,...) max_loop_us=...
```

BOOT events are reported separately. If the IMU cannot initialize, the runtime continues: matrix diagnostics still operate, telemetry reports the fault, and blinking red corner pixels mark the degraded state.

### BOOT controls

The diagnostic runtime mirrors the eventual one-button interaction contract:

- **short BOOT press:** reset the current diagnostic page/timer;
- **long BOOT press:** advance exactly one page while held; releasing it must not generate a short press.

The host-tested planning thresholds are <=600 ms for short and >=800 ms for long, with a deliberate ambiguity band between them.

### Diagnostic pages

Pages cycle in this order:

1. **Pixel sweep** — one amber logical pixel advances from index 0 through 63 every 100 ms. Record the physical path; this establishes the real matrix ordering.
2. **Primary colours** — full matrix requests red, green and blue in sequence every 700 ms at only 16/255 global brightness. Record the colours actually seen; this establishes RGB/GRB order.
3. **Gravity** — a cyan point follows the provisional matrix-plane projection of accelerometer X/Y. The current transform is deliberately explicit but unvalidated; this page is for discovering orientation, not asserting it.

All pages pass through `MatrixOutput`, which clamps output to the current development ceiling of 32/255. No diagnostic page or future scene should write the NeoPixel driver directly.

## Physical validation capture for ES-002

After flashing, paste serial output plus short observations into issue #2 or its PR.

Capture at least:

1. the complete `espsand.probe begin` through `espsand.probe end` block;
2. the `espsand.io start` line;
3. whether pixel index 0 starts where expected and the path taken by all 64 pixels;
4. whether the requested red -> green -> blue sequence is visually correct;
5. telemetry while holding the board in six static orientations: face-up, face-down, left edge down, right edge down, USB edge down, opposite edge down;
6. one short BOOT press and one long BOOT press;
7. approximately 10 seconds of telemetry to establish IMU rate/failure stability;
8. any PCB revision/date/lot marking visible on the physical board.

Do **not** use this short diagnostic as evidence for a safe sustained LED brightness. Thermal/current soak belongs to later power-policy validation.

## Source boundaries

- `lib/espsand_core/` is pure, host-testable C++ and contains board-independent I/O contracts plus deterministic input state machines.
- `src/board/` contains hardware-facing ESP32/Arduino adapters.
- `src/app/diagnostic_runtime.*` is the ES-002 non-blocking orchestration layer.
- `src/main.cpp` is the embedded entry point.
- `test/` contains host-native tests and fakes.
- `docs/hardware/BOARD_PROFILE.md` is the source of truth for hardware facts and evidence status.

Do not promote `ASSUMED` hardware values to `KNOWN` without vendor or physical evidence.
