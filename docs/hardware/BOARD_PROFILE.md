# ESPsand board profile

This file is the hardware source of truth for ESPsand v0.

Each field is explicitly classified as:

- **KNOWN** — supported directly by owner-provided identification or accepted vendor/physical evidence;
- **ASSUMED** — a reasonable working value for the identified product, but not yet confirmed on the owner's exact boards;
- **NEEDS_PHYSICAL_VALIDATION** — do not treat this as measured behaviour until evidence is captured.

## Identity and compute

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Owner inventory | KNOWN | Three target boards are physically available. |
| Product | KNOWN | Waveshare `ESP32-S3-Matrix`, SKU 27119. Owner supplied the target listing plus a matching front/rear product image; Waveshare's official documentation identifies SKU 27119 as `ESP32-S3-Matrix`. |
| PCB revision marking | NEEDS_PHYSICAL_VALIDATION | Capture any revision/date/lot marking visible on the actual boards before assuming all revisions route identically. |
| MCU | KNOWN | ESP32-S3; Waveshare's product page specifies ESP32-S3FH4R2 for SKU 27119. |
| CPU/core count | KNOWN | Xtensa 32-bit LX7 dual-core, up to 240 MHz. |
| Flash | KNOWN, vendor + physical boot evidence | 4 MB. On the first physical bring-up the ROM/runtime detected `4096k`; the original generic DevKitC profile incorrectly encoded 8 MB in the image header and prevented application startup. |
| PSRAM | KNOWN, vendor + physical runtime evidence | Waveshare specifies 2 MB PSRAM and the ES-002 foundation probe measured `2095103` usable bytes on the owner's board. |
| SRAM / ROM | KNOWN | Vendor documentation lists 512 KB SRAM, 384 KB ROM, 16 KB RTC SRAM. |
| Main regulator | KNOWN | ME6217C33M5G LDO; vendor identifies 800 mA maximum regulator current. This is **not** a safe continuous LED current budget. |

Primary references:

- `https://docs.waveshare.com/ESP32-S3-Matrix`
- `https://www.waveshare.com/esp32-s3-matrix.htm`
- `https://docs.waveshare.com/ESP32-S3-Matrix/Arduino`

Owner target listing:
`https://www.aliexpress.com/item/1005006962940633.html`

## Onboard resources confirmed for the target product

The supplied target image and official Waveshare material agree on the following onboard resources:

- USB Type-C connector;
- 8×8 / 64-pixel RGB LED matrix;
- BOOT button;
- RESET button;
- ME6217C33M5G LDO;
- QMI8658/QMI8658C-family QST 6-axis accelerometer + gyroscope;
- ESP32-S3 MCU;
- RGB-matrix `Dout` pad for extending the addressable chain;
- 17 GPIOs brought out around the board edges.

## PlatformIO target profile

PlatformIO's generic `esp32-s3-devkitc-1` board manifest describes an 8 MB/no-PSRAM DevKitC. ESPsand uses it only as a compiler/framework base and overrides the Waveshare ESP32-S3FH4R2 memory geometry in `platformio.ini`:

```ini
board_build.arduino.memory_type = qio_qspi
board_build.flash_mode = qio
board_build.psram_type = qio
board_build.partitions = default.csv
board_upload.flash_size = 4MB
board_upload.maximum_size = 4194304
```

`BOARD_HAS_PSRAM` is also defined for embedded builds. The original 8 MB image-header mismatch caused this physical failure before application startup:

```text
Detected size(4096k) smaller than the size in the binary image header(8192k)
assert failed: do_core_init startup.c:328 (flash_ret == ESP_OK)
```

After the full FH4R2 profile correction, the minimal bring-up firmware boots and runs continuously on the owner's physical board. Do not remove these overrides while the generic DevKitC board definition remains the base profile.

## Pin/profile map

| Signal | Status | Current value / evidence |
| --- | --- | --- |
| RGB matrix data | KNOWN, vendor + physical | GPIO14. Waveshare's official Arduino example defines `PIN_NEOPIXEL 14`, and the minimal physical bring-up successfully drives the onboard RGB chain through GPIO14. |
| QMI8658 SDA | KNOWN, physical | GPIO11. ES-002 communicates successfully with the onboard QMI8658 using this SDA routing. |
| QMI8658 SCL | KNOWN, physical | GPIO12. ES-002 communicates successfully with the onboard QMI8658 using this SCL routing. |
| QMI8658 INT1 | ASSUMED | GPIO10. ES-002 does not require interrupts. |
| QMI8658 INT2 | ASSUMED | GPIO13. ES-002 does not require interrupts. |
| BOOT button | KNOWN, physical | GPIO0. ES-002's configured GPIO0 input produced both short and long BOOT events on the owner's board. |
| BOOT active level | KNOWN, physical | active-low with internal pull-up; confirmed by successful ES-002 button events through the configured path. |
| Native USB D- / D+ | ASSUMED | GPIO19 / GPIO20; never repurpose while USB is required. |
| USB serial/JTAG device | KNOWN, physical | Windows enumerates the board as `USB VID:PID=303A:1001` and the corrected firmware emits continuous serial telemetry after host re-enumeration/reconnect. |

Secondary routing reference:
`https://devices.esphome.io/devices/waveshare-esp32s3-matrix/`

Waveshare publishes schematic/example resources from:
`https://docs.waveshare.com/ESP32-S3-Matrix/Resources-And-Documents`

## Matrix

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Geometry | KNOWN | 8×8 / 64 RGB emitters. |
| Extension output | KNOWN | Onboard `Dout` pad is provided for extending the RGB chain. |
| Control interface | KNOWN, vendor + physical | NeoPixel-compatible single-wire addressable chain; Waveshare's Arduino guide uses NeoPixel APIs on GPIO14 and the physical minimal probe cycles the onboard chain successfully. |
| Exact LED silicon | NEEDS_PHYSICAL_VALIDATION | Not required by ES-002 if the NeoPixel-compatible timing is correct. |
| Data GPIO | KNOWN, physical | GPIO14. Minimal `neopixelWrite()` bring-up visibly cycles the onboard RGB chain. |
| Pixel ordering | NEEDS_PHYSICAL_VALIDATION | ES-002 starts with linear row-major interpretation and provides a one-pixel sweep specifically to measure actual order. |
| RGB/GRB byte order | KNOWN, physical | RGB. With the original `NEO_GRB` adapter, requested `red -> green -> blue` appeared `green -> red -> blue`, and requested amber appeared lime. The adapter now uses `NEO_RGB`. |
| Development brightness ceiling | ASSUMED policy | ES-002 clamps all output to 32/255 or less and uses 16/255 for full-panel primary-colour frames. This is intentionally conservative, not a certified safe limit. |
| Practical sustained ceiling | NEEDS_PHYSICAL_VALIDATION | Vendor warns that excessive brightness can rapidly heat/damage the board. Establish later with measured soak evidence. |

The nominal 800 mA LDO rating must not be treated as an LED current budget.

## IMU

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Device family | KNOWN | QMI8658/QMI8658C-family QST six-axis accelerometer + gyroscope. |
| SDA/SCL | KNOWN, physical | GPIO11 / GPIO12. ES-002 successfully initializes and continuously polls the onboard sensor on these pins. |
| I2C address | KNOWN, physical | `0x6B`. ES-002 repeatedly detected the sensor there. |
| WHO_AM_I | KNOWN, physical | `0x05`. ES-002 validated this value before accepting the device. |
| Revision register | KNOWN, physical | `0x7C` on the tested unit. |
| INT1 / INT2 | ASSUMED | GPIO10 / GPIO13; unused by ES-002. |
| Configuration used by ES-002 | IMPLEMENTED + physically exercised | +/-8 g accelerometer and +/-512 dps gyro at 1 kHz sensor ODR with LPFs enabled; firmware nominally polls at 200 Hz. |
| Stable sample rate | KNOWN for current runtime | Approximately 180 Hz successful polling in the captured run, with zero read failures. |
| Matrix-relative in-plane orientation | KNOWN, physical visual calibration | Current screen-space transform is `matrix_x=-imu_y`, `matrix_y=+imu_x`, correcting the observed 90-degree rotation so visible gravity points downward on the matrix. |
| Full 3D board-centric orientation | NEEDS_PHYSICAL_VALIDATION | Complete six-pose mapping into USB/SIDE/FACE axes and exact Z/sign conventions remain to be measured. |

For human-facing terminology, future calibration and scene code should use board-centric axes rather than raw sensor names:

- **USB axis** — in the PCB plane along the USB-C cable/connector direction;
- **SIDE axis** — in the PCB plane perpendicular to USB, across the matrix left/right;
- **FACE axis** — normal to the PCB/LED face.

Raw QMI8658 X/Y/Z remain available internally. See `docs/hardware/CALIBRATION_2026-09-14.md` for the physical observations that established the current colour order and in-plane transform.

## BOOT and RESET buttons

The product has both BOOT and RESET buttons. Waveshare documents BOOT as the download-mode button used while resetting.

ES-002 uses BOOT as GPIO0 active-low and implements a pure, host-tested debounce/gesture state machine:

- <=600 ms stable press: short/reset event;
- >=800 ms stable press: long/next-diagnostic event emitted once while held;
- the 600–800 ms ambiguity band emits neither event;
- long release never emits an additional short event.

The physical run produced both short and long BOOT events through this GPIO0 active-low path, so the GPIO and active level are now treated as known for the tested board.

## Touch-capable candidates

The ESP32-S3 exposes native touch channels on GPIO1–GPIO14. Silicon capability does **not** mean a pad is free or physically useful on this PCB.

Current status: **NEEDS_PHYSICAL_VALIDATION**.

The target board exposes labelled edge GPIOs, making the no-added-components experiment plausible. GPIO10–GPIO14 are consumed by QMI8658/matrix functions and must not be repurposed. GPIO1–GPIO9 remain candidate touch channels until ES-003 reconciles schematic routing and measures them.

## ES-002 physical validation evidence

The physical bring-up has now established:

- actual 4 MB flash and approximately 2 MB PSRAM;
- corrected FH4R2 memory profile boots successfully;
- Arduino user code executes continuously;
- GPIO14 drives the onboard NeoPixel-compatible RGB chain;
- logical LED byte order is RGB, not GRB;
- USB serial/JTAG telemetry works after Windows completes USB re-enumeration/reconnect;
- QMI8658 responds on GPIO11/GPIO12 at `0x6B` with WHO_AM_I `0x05` and revision `0x7C`;
- current runtime sustains approximately 180 Hz IMU polling with zero captured read failures;
- BOOT GPIO0 active-low path produces both short and long events;
- visible gravity requires the in-plane projection `x=-raw_y`, `y=+raw_x`.

The PlatformIO monitor may briefly report `ClearCommError` / `PermissionError(13)` while the device re-enumerates, then reconnect to the same COM port. A reconnect followed by steady heartbeat is not a firmware failure.

Still capture:

1. physical pixel traversal/order of indices 0..63;
2. six controlled static poses to map raw X/Y/Z into USB/SIDE/FACE and confirm all signs;
3. any visible PCB revision/date/lot marking;
4. sustained thermal/current evidence for practical LED brightness ceilings;
5. whether the single observed TG0 watchdog reset recurs under normal operation.

Do not infer a sustained safe LED brightness from a short diagnostic run.
