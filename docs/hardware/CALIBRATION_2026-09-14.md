# Physical colour and in-plane axis calibration — 2026-09-14

Target: owner-held Waveshare ESP32-S3-Matrix (SKU 27119), LEDs facing the observer.

This note records direct physical observations used to correct ES-002's initial provisional matrix configuration.

## RGB byte order

The ES-002 primary-colour diagnostic requests logical `red -> green -> blue` frames. With the original Adafruit NeoPixel adapter configured as `NEO_GRB`, the owner observed:

```text
green -> red -> blue
```

The pixel-sweep diagnostic, which requests logical amber `{255, 96, 0}`, likewise appeared lime/green rather than amber. These two independent observations identify an R/G byte swap in the adapter configuration. The onboard chain therefore needs `NEO_RGB` for the logical `Rgb{r,g,b}` contract used by ESPsand.

## In-plane gravity orientation

With LEDs facing the owner, the original identity XY projection placed the gravity marker at centre-right when physical gravity should have been downward on the displayed panel. The owner described the required correction as rotating the displayed gravity result 90 degrees clockwise.

For the existing screen convention (`+x` right, `+y` down), that correction is:

```text
matrix_x = -imu_y
matrix_y = +imu_x
```

This is now the default in-plane projection used by ES-002. It corrects the observed display rotation, but it is not yet a full six-pose IMU calibration. Exact board-centric 3D axis naming/signs remain to be confirmed with controlled static poses.

## Human-facing board axes

For future calibration and scene semantics, use board-centric names rather than raw sensor X/Y/Z:

- **USB axis** — in the PCB plane, along the USB-C cable/connector direction;
- **SIDE axis** — in the PCB plane, perpendicular to USB, left/right across the matrix;
- **FACE axis** — normal to the PCB/LED face.

Raw QMI8658 X/Y/Z remain available internally. A later six-pose calibration should map them explicitly into USB/SIDE/FACE with signs.

## Other physical evidence from the same run

The normal ES-002 runtime successfully detected QMI8658 at `0x6B`, WHO_AM_I `0x05`, revision `0x7C`, and sustained approximately 180 Hz successful polling with zero read failures during the captured run. BOOT short and long events were also observed through the configured GPIO0 active-low input path.

Do not infer a sustained-safe LED brightness from this calibration session.
