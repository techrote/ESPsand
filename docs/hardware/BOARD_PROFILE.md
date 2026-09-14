# ESPsand board profile

This file is the hardware source of truth for ESPsand v0.

Each field is explicitly classified as:

- **KNOWN** — supported directly by owner-provided identification or accepted vendor evidence;
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
| Flash | KNOWN | 4 MB. Runtime probe should still confirm the physical units. |
| PSRAM | KNOWN (product specification) | Waveshare's product page specifies 2 MB PSRAM for SKU 27119. Runtime probe remains the physical-unit confirmation. |
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

## Pin/profile map

| Signal | Status | Current value / evidence |
| --- | --- | --- |
| RGB matrix data | KNOWN | GPIO14. Waveshare's official Arduino example defines `PIN_NEOPIXEL 14`. |
| QMI8658 SDA | ASSUMED | GPIO11; supported by public board-specific examples/community pinouts, pending vendor-schematic or physical confirmation. |
| QMI8658 SCL | ASSUMED | GPIO12; same evidence boundary as SDA. |
| QMI8658 INT1 | ASSUMED | GPIO10. ES-002 does not require interrupts. |
| QMI8658 INT2 | ASSUMED | GPIO13. ES-002 does not require interrupts. |
| BOOT button | ASSUMED | GPIO0. Strong ESP32-S3/Waveshare-family expectation; physical/schematic confirmation remains pending. |
| BOOT active level | ASSUMED | active-low with internal pull-up. |
| Native USB D- / D+ | ASSUMED | GPIO19 / GPIO20; never repurpose while USB is required. |

Secondary routing reference:
`https://devices.esphome.io/devices/waveshare-esp32s3-matrix/`

Waveshare publishes schematic/example resources from:
`https://docs.waveshare.com/ESP32-S3-Matrix/Resources-And-Documents`

## Matrix

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Geometry | KNOWN | 8×8 / 64 RGB emitters. |
| Extension output | KNOWN | Onboard `Dout` pad is provided for extending the RGB chain. |
| Control interface | KNOWN | NeoPixel-compatible single-wire addressable chain; Waveshare's Arduino guide uses NeoPixel APIs on GPIO14. |
| Exact LED silicon | NEEDS_PHYSICAL_VALIDATION | Not required by ES-002 if the NeoPixel-compatible timing is correct. |
| Data GPIO | KNOWN | GPIO14. |
| Pixel ordering | NEEDS_PHYSICAL_VALIDATION | ES-002 starts with linear row-major interpretation and provides a one-pixel sweep specifically to measure actual order. |
| RGB/GRB byte order | NEEDS_PHYSICAL_VALIDATION | ES-002 starts with `NEO_GRB`; the primary-colour diagnostic is the acceptance test. |
| Development brightness ceiling | ASSUMED policy | ES-002 clamps all output to 32/255 or less and uses 16/255 for full-panel primary-colour frames. This is intentionally conservative, not a certified safe limit. |
| Practical sustained ceiling | NEEDS_PHYSICAL_VALIDATION | Vendor warns that excessive brightness can rapidly heat/damage the board. Establish later with measured soak evidence. |

The nominal 800 mA LDO rating must not be treated as an LED current budget.

## IMU

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Device family | KNOWN | QMI8658/QMI8658C-family QST six-axis accelerometer + gyroscope. |
| SDA/SCL | ASSUMED | GPIO11 / GPIO12. |
| I2C address | NEEDS_PHYSICAL_VALIDATION | ES-002 probes `0x6B` first and `0x6A` second, then reports the detected address. |
| Expected WHO_AM_I | ASSUMED from QMI8658 driver/spec lineage | `0x05`; ES-002 refuses to initialize a candidate address unless this value is read. |
| INT1 / INT2 | ASSUMED | GPIO10 / GPIO13; unused by ES-002. |
| Configuration used by ES-002 | IMPLEMENTED, awaiting physical validation | +/-8 g accelerometer and +/-512 dps gyro at 1 kHz sensor ODR with LPFs enabled; firmware polls at 200 Hz. |
| Matrix-relative axis orientation | NEEDS_PHYSICAL_VALIDATION | ES-002 uses an explicit provisional identity XY transform and exposes raw/scaled values over serial. |
| Stable sample rate | NEEDS_PHYSICAL_VALIDATION | Runtime telemetry reports measured successful poll rate; target is approximately 200 Hz. |

The minimal ES-002 driver intentionally avoids a large sensor dependency: it probes WHO_AM_I, configures the small register subset needed for raw accel/gyro, and exposes both signed raw counts and scaled `g` / `dps` values.

## BOOT and RESET buttons

The product has both BOOT and RESET buttons. Waveshare documents BOOT as the download-mode button used while resetting.

ES-002 treats BOOT as GPIO0 active-low **provisionally** and implements a pure, host-tested debounce/gesture state machine:

- <=600 ms stable press: short/reset event;
- >=800 ms stable press: long/next-diagnostic event emitted once while held;
- the 600–800 ms ambiguity band emits neither event;
- long release never emits an additional short event.

Physical confirmation of GPIO0/active-low remains required before those fields become `KNOWN`.

## Touch-capable candidates

The ESP32-S3 exposes native touch channels on GPIO1–GPIO14. Silicon capability does **not** mean a pad is free or physically useful on this PCB.

Current status: **NEEDS_PHYSICAL_VALIDATION**.

The target board exposes labelled edge GPIOs, making the no-added-components experiment plausible. If the current routing assumptions are correct, GPIO10–GPIO14 are consumed by QMI8658/matrix functions and must not be repurposed. GPIO1–GPIO9 remain candidate touch channels until ES-003 reconciles schematic routing and measures them.

## ES-002 physical validation evidence

Flash the ES-002 diagnostic runtime using `docs/DEVELOPMENT.md`. Return the following evidence before promoting remaining assumptions:

1. complete startup probe block;
2. `espsand.io start ...` line, especially detected IMU address / WHO_AM_I / revision;
3. pixel-sweep observation: physical order of indices 0..63;
4. primary-colour observation: whether requested red, green, blue appear correctly;
5. raw/scaled IMU telemetry with the board held in at least face-up, face-down, left-edge-down, right-edge-down, USB-edge-down and opposite-edge-down orientations;
6. BOOT short/long behaviour;
7. reported IMU successful poll rate and any failure count;
8. any visible PCB revision/date/lot marking.

Do not infer a sustained safe LED brightness from a short diagnostic run.
