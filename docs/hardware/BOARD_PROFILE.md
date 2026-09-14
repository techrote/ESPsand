# ESPsand board profile

This file is the hardware source of truth for ESPsand v0.

Each field is explicitly classified as:

- **KNOWN** — supported directly by owner-provided identification, vendor documentation/schematic, or accepted physical evidence;
- **ASSUMED** — a reasonable working value for the identified product, but not yet confirmed strongly enough;
- **NEEDS_PHYSICAL_VALIDATION** — do not treat this as measured behaviour until evidence is captured.

## Identity and compute

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Owner inventory | KNOWN | Three target boards are physically available. |
| Product | KNOWN | Waveshare `ESP32-S3-Matrix`, SKU 27119. Owner-supplied target listing/images match Waveshare's official documentation. |
| PCB revision marking | NEEDS_PHYSICAL_VALIDATION | Capture any revision/date/lot marking visible on the actual boards before assuming all revisions route identically. |
| MCU | KNOWN | ESP32-S3FH4R2. |
| CPU/core count | KNOWN | Xtensa 32-bit LX7 dual-core, up to 240 MHz. |
| Flash | KNOWN, vendor + physical boot evidence | 4 MB. The first physical bring-up detected `4096k`; the original generic DevKitC 8 MB header prevented application startup. |
| PSRAM | KNOWN, vendor + physical runtime evidence | 2 MB nominal; the ES-002 foundation probe measured `2095103` usable bytes on the owner's board. |
| SRAM / ROM | KNOWN | Vendor documentation lists 512 KB SRAM, 384 KB ROM, 16 KB RTC SRAM. |
| Main regulator | KNOWN | ME6217C33M5G LDO; vendor identifies 800 mA maximum regulator current. This is **not** a safe continuous LED current budget. |

Primary references:

- `https://docs.waveshare.com/ESP32-S3-Matrix`
- `https://www.waveshare.com/esp32-s3-matrix.htm`
- `https://docs.waveshare.com/ESP32-S3-Matrix/Arduino`
- official schematic: `https://files.waveshare.com/wiki/ESP32-S3-Matrix/ESP32-S3-Matrix-Sch.pdf`

Owner target listing:
`https://www.aliexpress.com/item/1005006962940633.html`

## Onboard resources confirmed for the target product

- USB Type-C connector;
- 8×8 / 64-pixel RGB LED matrix;
- BOOT and RESET buttons;
- ME6217C33M5G LDO;
- QMI8658/QMI8658C-family QST 6-axis accelerometer + gyroscope;
- ESP32-S3 MCU;
- RGB-matrix `Dout` extension pad;
- 17 GPIOs brought out around the board edges.

## PlatformIO target profile

PlatformIO's generic `esp32-s3-devkitc-1` manifest is used only as the compiler/framework base. ESPsand overrides the Waveshare ESP32-S3FH4R2 memory geometry in `platformio.ini`:

```ini
board_build.arduino.memory_type = qio_qspi
board_build.flash_mode = qio
board_build.psram_type = qio
board_build.partitions = default.csv
board_upload.flash_size = 4MB
board_upload.maximum_size = 4194304
```

`BOARD_HAS_PSRAM` is defined for embedded builds. The original generic profile caused:

```text
Detected size(4096k) smaller than the size in the binary image header(8192k)
assert failed: do_core_init startup.c:328 (flash_ret == ESP_OK)
```

After the full FH4R2 correction, the board boots and runs continuously. Do not remove these overrides while the generic DevKitC definition remains the base profile.

## Pin/profile map

| Signal | Status | Current value / evidence |
| --- | --- | --- |
| RGB matrix data | KNOWN, vendor + physical | GPIO14; vendor example and physical output agree. |
| QMI8658 SDA | KNOWN, schematic + physical | GPIO11. |
| QMI8658 SCL | KNOWN, schematic + physical | GPIO12. |
| QMI8658 INT1 | KNOWN, vendor schematic | GPIO10; currently unused by runtime. |
| QMI8658 INT2 | KNOWN, vendor schematic | GPIO13; currently unused by runtime. |
| BOOT button | KNOWN, physical | GPIO0; short and long events physically observed. |
| BOOT active level | KNOWN, physical | Active-low with internal pull-up. |
| Native USB D- / D+ | KNOWN, vendor schematic | GPIO19 / GPIO20; never repurpose while native USB is required. |
| USB serial/JTAG device | KNOWN, physical | Windows enumerates `USB VID:PID=303A:1001`; corrected firmware emits continuous serial telemetry after host re-enumeration. |

## Matrix

The intended primary human-facing orientation is **USB up, LEDs facing the observer**. Matrix coordinates use that orientation: `+x` right and `+y` down.

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Geometry | KNOWN | 8×8 / 64 RGB emitters. |
| Extension output | KNOWN | Onboard `Dout` pad extends the addressable chain. |
| Control interface | KNOWN, vendor + physical | NeoPixel-compatible single-wire addressable chain on GPIO14. |
| Exact LED silicon | NEEDS_PHYSICAL_VALIDATION | Not required while compatible timing is confirmed. |
| Pixel ordering | KNOWN, physical | Linear row-major: top-left is index 0; each row advances left-to-right, then the next row begins at its left edge. `index = y * 8 + x`. |
| RGB/GRB byte order | KNOWN, physical | RGB. `NEO_GRB` visibly swapped requested red/green while blue remained correct; `NEO_RGB` was physically confirmed. |
| Development brightness ceiling | ASSUMED policy | All output is centrally clamped to 32/255 or less; full-panel primary diagnostics use 16/255. |
| Practical sustained ceiling | NEEDS_PHYSICAL_VALIDATION | Vendor warns that excessive brightness can rapidly heat/damage the board. Establish by later soak evidence. |

The nominal 800 mA LDO rating must not be treated as an LED current budget.

## IMU

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Device family | KNOWN | QMI8658/QMI8658C-family six-axis accelerometer + gyroscope. |
| SDA/SCL | KNOWN, schematic + physical | GPIO11 / GPIO12. |
| I2C address | KNOWN, physical | `0x6B`. |
| WHO_AM_I | KNOWN, physical | `0x05`. |
| Revision register | KNOWN, physical | `0x7C` on the tested unit. |
| INT1 / INT2 | KNOWN, vendor schematic | GPIO10 / GPIO13; unused by current runtime. |
| Configuration | IMPLEMENTED + physically exercised | +/-8 g accelerometer and +/-512 dps gyro at 1 kHz sensor ODR with LPFs; firmware nominally polls at 200 Hz. |
| Stable sample rate | KNOWN for current runtime | Approximately 180 Hz successful polling in captured runs, with zero captured read failures. |
| Matrix-relative in-plane orientation | KNOWN, physical | `matrix_x=-imu_y`, `matrix_y=+imu_x`; corrected visual gravity mapping physically confirmed. |
| Full 3D board-centric orientation | NEEDS_PHYSICAL_VALIDATION | Six-pose mapping into USB/SIDE/FACE axes and exact Z/sign conventions remain to be measured. |

Human-facing board-axis terminology:

- **USB axis** — in the PCB plane along the USB-C connector/cable direction;
- **SIDE axis** — in the PCB plane perpendicular to USB, across the matrix left/right;
- **FACE axis** — normal to the PCB/LED face.

In the intended primary orientation, USB is physically up, SIDE spans viewer-left to viewer-right, and FACE points toward/away from the observer. Raw QMI8658 X/Y/Z remain available internally.

## BOOT and RESET buttons

ESPsand uses BOOT as GPIO0 active-low. The host-tested gesture state machine is:

- <=600 ms stable press: short/reset event;
- >=800 ms stable press: long/next-diagnostic event emitted once while held;
- 600–800 ms ambiguity band: no event;
- release after a long press does not emit an additional short event.

Both short and long events were physically observed through this path.

## Touch-capable candidates — ES-003

The ESP32-S3 has native touch sensing on GPIO1–GPIO14, but board routing determines which channels are actually safe and useful.

Waveshare's official schematic plus the physical edge labels establish the no-component characterization set:

| GPIO | Routing/capability status | ES-003 result |
| ---: | --- | --- |
| 1 | KNOWN: exposed header, native touch | diagnostic local response only |
| 2 | KNOWN: exposed header, native touch | diagnostic local response only |
| 3 | KNOWN: exposed header, native touch | diagnostic local response only |
| 4 | KNOWN: exposed header, native touch | diagnostic centre/local response only |
| 5 | KNOWN: exposed header, native touch | diagnostic local response only |
| 6 | KNOWN: exposed header, native touch | diagnostic local response only |
| 7 | KNOWN: exposed header, native touch | diagnostic local response only |
| 8–9 | Native touch in silicon but not exposed on this board header | not useful as bare-board electrodes |
| 10 | QMI8658 INT1 | excluded |
| 11 | QMI8658 SDA | excluded |
| 12 | QMI8658 SCL | excluded |
| 13 | QMI8658 INT2 | excluded |
| 14 | RGB matrix data | excluded |

GPIO1–GPIO7 are KNOWN safe touch candidates. Physical ES-003 testing resolved the bare-board capability as **combo-only/common-mode useful** rather than two independent zones.

Observed behaviour on the tested board:

- swiping along the GPIO1..7 edge moves the local-response indication smoothly across the matrix;
- a normal fingertip spans most or all of the edge, so targeting one pad or cleanly separating two regions is impractical;
- pinching along the PCB edge gives the strongest and most repeatable response;
- the common-mode response tracks broad fingertip/PCB contact area well enough to provide a useful bounded intensity;
- local normalized channels sometimes flash amber spuriously, so local A/B events are not exposed to scenes.

The product mapping is therefore `cap_combo`/`event_combo` only. It uses the common-mode channel directly; local GPIO1..7 activity remains diagnostic and cannot directly trigger a product capacitive event. `cap_a` and `cap_b` remain zero on this board profile.

This is not a force or pressure measurement. Contact area, grip, moisture, grounding and other environmental effects can all change capacitive coupling.

See `docs/hardware/TOUCH_CHARACTERIZATION.md` for the detailed evidence boundary. The `NullTouchZones` adapter remains available so no scene depends on touch success.

## Radio / thermal policy

ESPsand v0 has no networking runtime requirement. Startup enforces Wi-Fi off/uninitialized and Bluetooth/BLE off, releasing the Bluetooth controller reservation where available. Native USB Serial/JTAG, IMU, watchdogs, brownout protection and PSRAM remain enabled. This prevents accidental radio activity but does not replace the central LED brightness/current policy.

## Physical validation evidence completed

The board bring-up has established:

- actual 4 MB flash and approximately 2 MB PSRAM;
- corrected FH4R2 memory profile boots successfully;
- Arduino user code executes continuously;
- GPIO14 drives the onboard addressable RGB matrix;
- logical LED byte order is RGB;
- matrix order is linear row-major in the intended USB-up orientation;
- USB Serial/JTAG telemetry works after Windows USB re-enumeration/reconnect;
- QMI8658 communicates on GPIO11/GPIO12 at `0x6B`, WHO_AM_I `0x05`, revision `0x7C`;
- approximately 180 Hz IMU polling with zero captured read failures;
- BOOT GPIO0 active-low short/long handling;
- corrected in-plane gravity transform `x=-raw_y`, `y=+raw_x`;
- bare-board GPIO1..7 capacitive sensing is useful as a deliberate edge-pinch/common-mode gesture, while reliable local A/B discrimination is not supported.

The PlatformIO monitor may briefly report `ClearCommError` / `PermissionError(13)` while native USB re-enumerates, then reconnect to the same COM port. A reconnect followed by steady telemetry is not a firmware failure.

## Remaining physical evidence

1. six controlled static poses to complete raw X/Y/Z -> USB/SIDE/FACE mapping;
2. visible PCB revision/date/lot marking if present;
3. sustained thermal/current evidence for practical LED brightness ceilings;
4. whether the single previously observed TG0 watchdog reset recurs under normal operation.

Do not infer a sustained safe LED brightness from short diagnostics.
