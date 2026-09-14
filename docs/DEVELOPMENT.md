# Development and verification

ESPsand v0 uses a pinned PlatformIO/Python toolchain so firmware compilation and host-native tests can be reproduced on Windows, Linux, or macOS.

## Prerequisites

- Python 3.11 is the CI reference interpreter. Newer supported Python versions may also work.
- Git.
- For physical flashing, a USB data cable and an ESP32-S3-Matrix-class board.

Create and activate a virtual environment if desired, then install the pinned tools:

```text
python -m pip install -r requirements-dev.txt
```

The repository currently pins:

- PlatformIO Core `6.1.18`;
- PlatformIO Espressif32 platform `6.10.0`;
- PlatformIO native platform `1.2.1`;
- clang-format `18.1.8`.

The firmware compile profile intentionally uses PlatformIO's generic `esp32-s3-devkitc-1` board definition while the exact physical ESPsand board revision is still being validated. The profile is a reproducible compile target, not a claim that the physical board is a DevKitC.

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

The native environment compiles with `-Wall -Wextra -Wpedantic -Werror`. This is the foundation's lightweight static-analysis policy; no large linter suite is introduced yet.

## Flash the foundation probe

The ES-001 firmware is deliberately non-invasive: it does **not** drive the matrix, IMU bus, BOOT pin, or candidate touch GPIOs because those mappings have not yet been physically validated.

To build and upload:

```text
python -m platformio run -e esp32s3 -t upload
```

Then open the serial monitor:

```text
python -m platformio device monitor -b 115200
```

Reset the board if necessary. The probe repeats every 30 seconds and reports:

- detected ESP chip model/revision/core count;
- CPU frequency and SDK version;
- detected flash and PSRAM sizes;
- heap size;
- the current status of all provisional pin/profile assumptions.

Paste the complete `espsand.probe begin` through `espsand.probe end` block into the relevant issue or PR when physical validation is requested.

## Source boundaries

- `lib/espsand_core/` is pure, host-testable C++ and must not include Arduino/ESP headers.
- `src/board/` contains hardware-facing ESP32/Arduino code.
- `src/main.cpp` is the embedded entry point.
- `test/` contains host-native tests.
- `docs/hardware/BOARD_PROFILE.md` is the source of truth for hardware facts and their evidence status.

Do not promote `ASSUMED` hardware values to `KNOWN` without documentary or physical evidence.
