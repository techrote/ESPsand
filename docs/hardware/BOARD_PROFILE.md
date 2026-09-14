# ESPsand board profile

This file is the hardware source of truth for ESPsand v0.

Each field is explicitly classified as:

- **KNOWN** — supported directly by owner-provided identification or accepted evidence;
- **ASSUMED** — a reasonable working value from the likely product family, but not yet confirmed on the owner's exact boards;
- **NEEDS_PHYSICAL_VALIDATION** — do not depend on this value until evidence is captured.

## Identity and compute

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Owner inventory | KNOWN | Three ESP32-S3 8×8 RGB-matrix boards with QMI8658C-class IMU are physically available. |
| MCU family | KNOWN | ESP32-S3, from the owner's product identification. |
| Exact product/revision marking | NEEDS_PHYSICAL_VALIDATION | Read silkscreen/PCB revision or provide clear front/back photo. |
| Likely product family | ASSUMED | Waveshare `ESP32-S3-Matrix` / SKU 27119 or a compatible board matching the same feature set. |
| Flash | ASSUMED | Likely 4 MB for Waveshare SKU 27119. Confirm with the ES-001 runtime probe. |
| PSRAM | NEEDS_PHYSICAL_VALIDATION | Runtime probe reports detected size; do not assume it exists. |
| CPU/core count | NEEDS_PHYSICAL_VALIDATION | Expected ESP32-S3 dual-core up to 240 MHz; capture probe output from the actual board. |

Public reference used for the **ASSUMED** Waveshare-family values:
`https://docs.waveshare.com/ESP32-S3-Matrix`

## Provisional pin/profile map

These pins match public Waveshare ESP32-S3-Matrix examples/community pinouts closely enough to seed later diagnostics. They are **not yet physically verified on the owner's boards**.

| Signal | Status | Provisional value |
| --- | --- | ---: |
| RGB matrix data | ASSUMED | GPIO14 |
| QMI8658 SDA | ASSUMED | GPIO11 |
| QMI8658 SCL | ASSUMED | GPIO12 |
| QMI8658 INT1 | ASSUMED | GPIO10 |
| QMI8658 INT2 | ASSUMED | GPIO13 |
| BOOT button | ASSUMED | GPIO0 |
| BOOT active level | ASSUMED | active-low |
| Native USB D- / D+ | ASSUMED | GPIO19 / GPIO20; never repurpose while USB is required |

No ES-001 firmware drives any of these pins.

Secondary reference for the provisional pin map:
`https://devices.esphome.io/devices/waveshare-esp32s3-matrix/`

## Matrix

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Geometry | KNOWN | 8×8 / 64 RGB emitters from the owner's product identification. |
| Electrical protocol / exact LED part | ASSUMED | Addressable NeoPixel/WS281x-class chain; exact part still needs confirmation. |
| Data GPIO | ASSUMED | GPIO14. |
| Pixel ordering / serpentine direction | NEEDS_PHYSICAL_VALIDATION | Resolve in ES-002 with a one-pixel-at-a-time diagnostic. |
| RGB/GRB colour order | NEEDS_PHYSICAL_VALIDATION | Resolve in ES-002 using primary-colour frames. |
| Practical brightness ceiling | NEEDS_PHYSICAL_VALIDATION | Vendor warns that excessive brightness rapidly heats the board. Establish a conservative software limit only after measurement/soak. |

The vendor warning is not a numeric current budget. Do not infer a safe sustained brightness from the nominal regulator rating.

## IMU

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Device family | KNOWN | QMI8658C-class six-axis accel/gyro from owner product identification. |
| SDA/SCL | ASSUMED | GPIO11 / GPIO12. |
| I2C address | NEEDS_PHYSICAL_VALIDATION | Probe explicitly in ES-002; do not hard-code a claimed physical result here. |
| INT1 / INT2 | ASSUMED | GPIO10 / GPIO13. |
| WHO_AM_I / silicon identity | NEEDS_PHYSICAL_VALIDATION | Read through the IMU driver in ES-002. |
| Matrix-relative axis orientation | NEEDS_PHYSICAL_VALIDATION | Place board in known orientations and record signed raw acceleration. |
| Stable sample rate | NEEDS_PHYSICAL_VALIDATION | Measure on hardware once the driver exists. |

## BOOT button

GPIO0 active-low is a strong product-family assumption but remains **ASSUMED** until the owner confirms the exact board or ES-002 observes the input safely.

ES-002 will implement debounced short/long semantics only after the pin is reconciled against the exact board profile.

## Touch-capable candidates

The ESP32-S3 has native touch channels on GPIO1–GPIO14. However, capability at the silicon does **not** mean the corresponding pad is free or physically useful on this board.

Current status: **NEEDS_PHYSICAL_VALIDATION**.

If the provisional pin map is correct, GPIO10–GPIO14 are already consumed by QMI8658/matrix functions and must not be repurposed. GPIO1–GPIO9 are only *candidate silicon channels* until the exact exposed-pin routing is confirmed. ES-003 owns empirical touch characterization and may conclude that component-free touch is not useful.

## Foundation runtime probe

ES-001 adds a safe serial probe that touches no uncertain peripheral pin. Flash it using the procedure in `docs/DEVELOPMENT.md` and capture the complete probe block.

Minimum evidence to return:

```text
espsand.probe begin
...
chip.model=...
chip.revision=...
chip.cores=...
chip.cpu_mhz=...
memory.flash_bytes=...
memory.psram_bytes=...
...
espsand.probe end
```

Also report:

1. exact text/silkscreen visible on the front and rear PCB;
2. whether Windows/Linux enumerates the board as a USB serial device after flashing;
3. any product/revision label supplied by the seller.

The profile should then be updated in the same PR that promotes any field from `ASSUMED`/`NEEDS_PHYSICAL_VALIDATION` to `KNOWN`.
