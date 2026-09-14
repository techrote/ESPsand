# ESPsand board profile

This file is the hardware source of truth for ESPsand v0.

Each field is explicitly classified as:

- **KNOWN** — supported directly by owner-provided identification or accepted evidence;
- **ASSUMED** — a reasonable working value from the identified product family, but not yet confirmed on the owner's exact boards;
- **NEEDS_PHYSICAL_VALIDATION** — do not depend on this value until evidence is captured.

## Identity and compute

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Owner inventory | KNOWN | Three target boards are physically available. |
| Product | KNOWN | Waveshare `ESP32-S3-Matrix`, SKU 27119. Owner supplied the target listing plus a matching front/rear product image; Waveshare's official documentation identifies SKU 27119 as `ESP32-S3-Matrix`. |
| PCB revision marking | NEEDS_PHYSICAL_VALIDATION | Product identity is now resolved, but capture any revision/date/lot marking visible on the actual boards before assuming all revisions route identically. |
| MCU family | KNOWN | ESP32-S3. |
| CPU/core count | KNOWN | Xtensa 32-bit LX7 dual-core, up to 240 MHz, per official Waveshare product documentation for SKU 27119. |
| Flash | KNOWN | 4 MB, per official Waveshare product documentation for SKU 27119. Runtime probe should still confirm the physical units. |
| PSRAM | NEEDS_PHYSICAL_VALIDATION | Waveshare's feature list does not advertise PSRAM for SKU 27119. Runtime probe reports detected size; do not assume it exists. |
| SRAM / ROM | KNOWN | Vendor lists 512 KB SRAM, 384 KB ROM, 16 KB RTC SRAM. |
| Main regulator | KNOWN | ME6217C33M5G LDO; vendor identifies 800 mA maximum regulator current. This is **not** a safe continuous LED current budget. |

Primary product reference:
`https://docs.waveshare.com/ESP32-S3-Matrix`

Owner target listing:
`https://www.aliexpress.com/item/1005006962940633.html`

## Onboard resources confirmed for the target product

The supplied target image and official Waveshare page agree on the following onboard resources:

- USB Type-C connector;
- 8×8 / 64-pixel RGB LED matrix;
- BOOT button;
- RESET button;
- ME6217C33M5G LDO;
- QMI8658/QMI8658C-family QST 6-axis accelerometer + gyroscope;
- ESP32-S3 MCU;
- RGB-matrix `Dout` pad for extending the addressable chain;
- 17 GPIOs brought out around the board edges according to Waveshare's product description.

The exact PCB routing and revision-specific electrical details still belong to the schematic/physical-validation layer below.

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

Waveshare publishes a schematic and example bundle from:
`https://docs.waveshare.com/ESP32-S3-Matrix/Resources-And-Documents`

ES-002 should prefer those vendor resources over community pinouts when promoting any routing value to `KNOWN`, and should still preserve physical validation where behaviour depends on board revision.

## Matrix

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Geometry | KNOWN | 8×8 / 64 RGB emitters. |
| Extension output | KNOWN | Onboard `Dout` pad is provided by the target product for extending the RGB matrix chain. |
| Electrical protocol / exact LED part | ASSUMED | Addressable NeoPixel/WS281x-class chain; exact LED part still needs schematic/example confirmation. |
| Data GPIO | ASSUMED | GPIO14. |
| Pixel ordering / serpentine direction | NEEDS_PHYSICAL_VALIDATION | Resolve in ES-002 with a one-pixel-at-a-time diagnostic. |
| RGB/GRB colour order | NEEDS_PHYSICAL_VALIDATION | Resolve in ES-002 using primary-colour frames. |
| Practical brightness ceiling | NEEDS_PHYSICAL_VALIDATION | Vendor warns that excessive brightness rapidly heats the board and may damage it. Establish a conservative software limit only after measurement/soak. |

The vendor warning is not a numeric current budget. Do not infer a safe sustained brightness from the regulator's nominal 800 mA maximum rating.

## IMU

| Field | Status | Current value / evidence |
| --- | --- | --- |
| Device family | KNOWN | QMI8658/QMI8658C-family QST six-axis accelerometer + gyroscope on the target product. |
| SDA/SCL | ASSUMED | GPIO11 / GPIO12. |
| I2C address | NEEDS_PHYSICAL_VALIDATION | Probe explicitly in ES-002; do not hard-code a claimed physical result here. |
| INT1 / INT2 | ASSUMED | GPIO10 / GPIO13. |
| WHO_AM_I / silicon identity | NEEDS_PHYSICAL_VALIDATION | Read through the IMU driver in ES-002. |
| Matrix-relative axis orientation | NEEDS_PHYSICAL_VALIDATION | Place board in known orientations and record signed raw acceleration. |
| Stable sample rate | NEEDS_PHYSICAL_VALIDATION | Measure on hardware once the driver exists. |

## BOOT and RESET buttons

The target product definitely has both BOOT and RESET buttons. Waveshare documents BOOT as the download-mode button used while resetting.

`GPIO0` active-low remains an **ASSUMED electrical mapping** until the vendor schematic or ES-002 confirms it for this board revision. RESET is not a normal GPIO input; it should remain dedicated to reset behaviour.

ES-002 will implement debounced short/long BOOT semantics only after the pin is reconciled against the exact board profile.

## Touch-capable candidates

The ESP32-S3 has native touch channels on GPIO1–GPIO14. However, capability at the silicon does **not** mean the corresponding pad is free or physically useful on this board.

Current status: **NEEDS_PHYSICAL_VALIDATION**.

The target board exposes labelled edge GPIOs, making the no-added-components experiment plausible. If the provisional pin map is correct, GPIO10–GPIO14 are already consumed by QMI8658/matrix functions and must not be repurposed. GPIO1–GPIO9 are only *candidate silicon channels* until the exact routing is reconciled against the Waveshare schematic and ES-003 measurements.

ES-003 owns empirical touch characterization and may conclude that component-free touch is not useful.

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

1. any PCB revision/date/lot text visible on the front or rear beyond the `ESP32-S3-Matrix` product marking;
2. whether Windows/Linux enumerates the board as a USB serial device after flashing;
3. the complete probe block above.

Product identity no longer needs to be rediscovered: the v0 target is Waveshare `ESP32-S3-Matrix` SKU 27119. Remaining promotion from `ASSUMED`/`NEEDS_PHYSICAL_VALIDATION` to `KNOWN` should be based on vendor schematic/example evidence and/or actual-board measurements.