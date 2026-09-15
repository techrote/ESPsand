# ESPsand v0 hardware target and characterization

## Current identification state

The tested target is the Waveshare ESP32-S3-Matrix family board used by this repository: ESP32-S3, onboard 8×8 RGB chain, QMI8658-class IMU, BOOT button and USB. Three boards are available; v0 remains single-board only. `docs/hardware/BOARD_PROFILE.md` is the field-by-field hardware evidence record.

## Foundation compile profile

PlatformIO's generic `esp32-s3-devkitc-1` definition is used only as a compiler/Arduino-core base. The repository overrides the physical target's FH4R2 memory profile: 4 MB flash plus 2 MB PSRAM.

## Hardware abstraction boundary

Board drivers do not own simulation semantics. The pure core defines narrow interfaces for clock, IMU, button, optional touch input, matrix output and diagnostics. `lib/espsand_core/` remains free of Arduino/ESP headers.

The RGB chain is physically calibrated as GPIO14, RGB byte order, linear row-major pixel order in the documented primary orientation. `MatrixOutput` is the sole NeoPixel owner and physical write gateway.

## IMU requirements

Raw acceleration/gyro remain the hardware source. Board-independent input work derives projected gravity and motion semantics before model code receives normalized values. The calibrated in-plane transform is explicit and host-tested.

## LED constraints — ES-005 policy

The 64 onboard RGB LEDs can produce substantial aggregate load and heating. No scene may request arbitrary physical brightness directly.

ES-005 adds a two-stage centralized software budget inside the physical `MatrixOutput` gateway:

1. a hard global brightness ceiling;
2. a deterministic aggregate RGB PWM-load envelope that can reduce dense frames further.

The current provisional defaults are:

- hard brightness ceiling: **32/255**;
- aggregate frame-load limit: **4096 dimensionless software load units**.

The load unit is `sum(R+G+B) * brightness / 255`; it is not milliamps. The value 4096 is deliberately a conservative development envelope, not a measured current or thermal rating. It allows sparse bright features to retain contrast while making dense full-white output dimmer than the hard ceiling.

The board gateway will not allow runtime policy to raise brightness above 32/255 while that hard ceiling remains unvalidated. A later evidence-backed hardware change may revise it deliberately.

### Thermal/current evidence boundary

The vendor warning about excessive brightness establishes a qualitative hazard but no numeric safe limit. Physical validation is therefore still required before documenting a sustained safe ceiling.

Use representative scenes and dense stress patterns, preferably with an inline USB current meter and temperature probe, and soak candidate settings for an extended period. Record pattern, ambient conditions, applied limiter state, duration, current/temperature where available, and any resets/USB instability/colour shift. Stop on undesirable heating. Do not promote a brief visual test to a safe sustained-current claim.

## Capacitive experiment result

GPIO1–GPIO7 are safe exposed native-touch candidates on the tested board. Bare-board characterization supports broad `cap_combo`, a pinch-gated coarse slider, and an explicit bounded external noise impulse. Reliable independent A/B button semantics are not exposed. Missing/noisy capacitive sensing remains optional and degrades cleanly.

## Known routing summary

- RGB matrix data: GPIO14;
- QMI8658 SDA/SCL: GPIO11/GPIO12;
- QMI8658 interrupts: GPIO10/GPIO13;
- BOOT: GPIO0 active-low;
- USB D-/D+: GPIO19/GPIO20;
- safe exposed touch candidates: GPIO1–GPIO7.

Do not repurpose matrix, IMU, BOOT or USB pins for scene logic.
