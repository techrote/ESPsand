# Minimal ESP32-S3-Matrix bring-up probe

This target exists to isolate basic user-code execution from the ES-002 runtime after the first physical validation attempt produced neither matrix output nor serial telemetry.

## Physical evidence collected

On 2026-09-14 the owner successfully flashed the ES-002 image to the target Waveshare ESP32-S3-Matrix over `COM7`. `esptool` completed all writes, verified hashes, and reported a successful hard reset. PlatformIO subsequently enumerated:

```text
COM7
Hardware ID: USB VID:PID=303A:1001 SER=F0:F5:BD:75:84:BC
Description: USB Serial Device (COM7)
```

Opening the 115200-baud monitor and pressing RESET initially produced no ESPsand serial output and no LEDs.

The first minimal bring-up pass exposed the first hard failure:

```text
spi_flash: Detected size(4096k) smaller than the size in the binary image header(8192k). Probe failed.
assert failed: do_core_init startup.c:328 (flash_ret == ESP_OK)
```

That established that the physical target really contains 4 MB flash while the generic PlatformIO `esp32-s3-devkitc-1` profile is an N8 / 8 MB profile.

After overriding only `board_upload.flash_size = 4MB`, the explicit 8 MB-vs-4 MB assertion disappeared, but the board still entered an immediate software-reset loop before application output:

```text
ESP-ROM:esp32s3-20210327
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x403cdb0a
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3808,len:0x4bc
load:0x403c9700,len:0xbd8
load:0x403cc700,len:0x2a0c
entry 0x403c98d0
```

This second result is important: the flash-capacity header mismatch was real and is now gone, but changing only that one field was incomplete. PlatformIO's `esp32-s3-devkitc-1` manifest also selects an 8 MB partition table and describes an N8/no-PSRAM device, while the Waveshare target is specified as ESP32-S3FH4R2: 4 MB quad flash + 2 MB quad PSRAM.

The common ESP32-S3 environment now overrides the complete memory profile:

- `board_build.arduino.memory_type = qio_qspi`
- `board_build.flash_mode = qio`
- `board_build.psram_type = qio`
- `board_build.partitions = default.csv` (4 MB-compatible Arduino partition table)
- `board_upload.flash_size = 4MB`
- `board_upload.maximum_size = 4194304`
- `BOARD_HAS_PSRAM`

This is deliberately a target-profile correction, not a peripheral change.

## What the minimal target removes

`esp32s3_bringup` does not instantiate the ES-002 runtime, matrix library adapter, QMI8658 driver, button logic, scheduler, or diagnostics stack.

It does only two things:

1. calls Waveshare's documented `neopixelWrite()` path directly on GPIO14 and cycles one chain pixel through low-brightness red, green, blue, off once per second;
2. starts Arduino `Serial` and emits a heartbeat once per second.

The probe uses brightness `8/255`, far below showcase levels.

## Windows / COM7 procedure

Because the partition table has changed from the generic 8 MB default to the correct 4 MB table, use a clean build and erase flash before the next test so stale partition/coredump data cannot survive:

```powershell
cd C:\Users\-\ESPsand
git pull
python -m platformio run -e esp32s3_bringup -t clean
python -m platformio run -e esp32s3_bringup -t erase --upload-port COM7
python -m platformio run -e esp32s3_bringup -t upload --upload-port COM7
python -m platformio device monitor -e esp32s3_bringup -p COM7 -b 115200
```

If erase/upload cannot enter download mode automatically, hold BOOT, tap RESET, release BOOT, then retry.

The environment sets `monitor_dtr = 0` and `monitor_rts = 0` so opening the monitor does not deliberately manipulate those control lines.

## Expected evidence

Serial, if working, should contain:

```text
espsand.bringup boot
espsand.bringup gpio14=vendor_neopixel serial=hwcdc
espsand.bringup alive ms=... phase=...
```

Visually, at least the first RGB-chain LED should cycle:

```text
red -> green -> blue -> off -> repeat
```

## Interpretation after the full FH4R2 profile fix

| Observation | Meaning |
| --- | --- |
| LED cycles and serial heartbeat appears | Basic boot, GPIO14, Arduino runtime and serial path work; return to ES-002 diagnostics. |
| LED cycles but serial is silent | User code is executing and GPIO14 is valid; investigate USB CDC/monitor configuration. |
| Serial heartbeat appears but LED is silent | User code and serial work; investigate RGB electrical/protocol/chain assumptions. |
| Immediate reset loop remains | Memory-profile mismatch was not the only startup problem; capture the exact reset block and continue below Arduino `setup()`/core-init level. |
| The 8192k-vs-4096k assertion returns | Local checkout/build cache is stale or the corrected profile was not used. |

Do not promote unresolved peripheral fields to `KNOWN` solely because this probe compiles. Physical observations remain authoritative.
