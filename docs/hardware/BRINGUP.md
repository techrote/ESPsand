# Minimal ESP32-S3-Matrix bring-up probe

This target exists to isolate basic user-code execution from the ES-002 runtime after the first physical validation attempt produced neither matrix output nor serial telemetry.

## Physical evidence that triggered this probe

On 2026-09-14 the owner successfully flashed the ES-002 image to the target Waveshare ESP32-S3-Matrix over `COM7`. `esptool` completed all writes, verified hashes, and reported a successful hard reset. PlatformIO subsequently enumerated:

```text
COM7
Hardware ID: USB VID:PID=303A:1001 SER=F0:F5:BD:75:84:BC
Description: USB Serial Device (COM7)
```

However, opening the 115200-baud monitor and pressing RESET produced no ESPsand serial output, and no LEDs lit. Normal board warmth was observed.

The minimal bring-up target then exposed the actual boot failure immediately:

```text
spi_flash: Detected size(4096k) smaller than the size in the binary image header(8192k). Probe failed.
assert failed: do_core_init startup.c:328 (flash_ret == ESP_OK)
```

This establishes the root cause of the original silence: the generic PlatformIO `esp32-s3-devkitc-1` profile generated an 8 MB image header for a physical 4 MB target. The application never reached `setup()`. Repeated core-dump CRC messages observed afterward are secondary effects of the reboot loop and are not evidence that the physical flash is corrupt.

`platformio.ini` now overrides `board_upload.flash_size = 4MB` for the common ESP32-S3 environment. PlatformIO's Espressif32 builder uses that manifest value both when creating the image header and when invoking `esptool`, so both the normal ES-002 firmware and `esp32s3_bringup` inherit the physical target size.

## What the minimal target removes

`esp32s3_bringup` does not instantiate the ES-002 runtime, matrix library adapter, QMI8658 driver, button logic, scheduler, or diagnostics stack.

It does only two things:

1. calls Waveshare's documented `neopixelWrite()` path directly on GPIO14 and cycles one chain pixel through low-brightness red, green, blue, off once per second;
2. starts Arduino `Serial` on the Hardware CDC/JTAG configuration and emits a heartbeat once per second.

The probe uses brightness `8/255`, far below showcase levels.

## Windows / COM7 procedure

From the repository root after pulling the 4 MB profile fix:

```powershell
python -m platformio run -e esp32s3_bringup -t clean
python -m platformio run -e esp32s3_bringup -t upload --upload-port COM7
python -m platformio device monitor -e esp32s3_bringup -p COM7 -b 115200
```

A clean rebuild is required for this re-test so no previously generated 8 MB image survives in `.pio/build`.

The environment sets `monitor_dtr = 0` and `monitor_rts = 0` so opening the monitor does not deliberately manipulate those control lines.

If necessary, tap RESET once after the monitor is open.

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

## Interpretation after the 4 MB fix

| Observation | Meaning |
| --- | --- |
| LED cycles and serial heartbeat appears | Basic boot, GPIO14, Arduino runtime and HWCDC all work; return to ES-002 diagnostics. |
| LED cycles but serial is silent | User code is executing and GPIO14 is valid; investigate USB CDC/monitor configuration. |
| Serial heartbeat appears but LED is silent | User code and USB CDC work; investigate RGB electrical/protocol/chain assumptions. |
| Neither LED nor serial appears, but no flash-size assertion is present | Investigate the next boot/runtime layer rather than the already-resolved flash-capacity mismatch. |
| The 8192k-vs-4096k assertion still appears | The local checkout/build cache does not contain the fixed profile; clean, pull, rebuild and verify `board_upload.flash_size = 4MB`. |

Do not promote any unresolved board-profile field to `KNOWN` solely because this probe compiles. Only physical observations count.
