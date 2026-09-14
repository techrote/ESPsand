# Physical colour, pixel-order and in-plane axis calibration — 2026-09-14

Target: owner-held Waveshare ESP32-S3-Matrix (SKU 27119).

This note records direct physical observations used to correct and validate ES-002's initial provisional matrix configuration.

## Primary physical orientation

The intended human-facing primary orientation is:

- board held vertically;
- USB-C connector/cable at the **top**;
- LEDs facing the observer.

Unless explicitly stated otherwise, matrix directions and pixel coordinates in this document use that orientation.

## RGB byte order

The ES-002 primary-colour diagnostic requests logical `red -> green -> blue` frames. With the original Adafruit NeoPixel adapter configured as `NEO_GRB`, the owner observed:

```text
green -> red -> blue
```

The pixel-sweep diagnostic, which requests logical amber `{255, 96, 0}`, likewise appeared lime/green rather than amber. These two independent observations identify an R/G byte swap in the adapter configuration. The onboard chain therefore needs `NEO_RGB` for the logical `Rgb{r,g,b}` contract used by ESPsand.

After changing the adapter to `NEO_RGB`, the owner confirmed that the primary colours and amber sweep appear as expected.

## Physical pixel traversal order

With the board in the primary orientation (USB up, LEDs facing the observer), the single-pixel amber sweep physically traverses the matrix as follows:

1. starts at the **top-left** pixel;
2. travels left-to-right across the complete top row;
3. continues at the **leftmost pixel of row 2** and travels left-to-right again;
4. repeats that pattern for each subsequent row through the bottom row.

Therefore the physical chain order is ordinary row-major, not serpentine:

```text
index = y * 8 + x
```

with `x=0..7` increasing left-to-right and `y=0..7` increasing top-to-bottom in the primary orientation. This matches ESPsand's existing `pixel_index()` convention, so no software remap is required for the tested board.

## In-plane gravity orientation

With LEDs facing the owner, the original identity XY projection placed the gravity marker at centre-right when physical gravity should have been downward on the displayed panel. The owner described the required correction as rotating the displayed gravity result 90 degrees clockwise.

For the existing screen convention (`+x` right, `+y` down), that correction is:

```text
matrix_x = -imu_y
matrix_y = +imu_x
```

This is now the default in-plane projection used by ES-002. After applying it, the owner confirmed that visible gravity is properly mapped.

This is an in-plane calibration, not yet a complete six-pose IMU calibration. Exact board-centric 3D axis naming/signs remain to be confirmed with controlled static poses.

## Human-facing board axes

For future calibration and scene semantics, use board-centric names rather than raw sensor X/Y/Z:

- **USB axis** — in the PCB plane, along the USB-C cable/connector direction;
- **SIDE axis** — in the PCB plane, perpendicular to USB, left/right across the matrix;
- **FACE axis** — normal to the PCB/LED face.

In the primary orientation, USB is physically up, SIDE spans viewer-left to viewer-right, and FACE points toward/away from the observer. Raw QMI8658 X/Y/Z remain available internally. A later six-pose calibration should map them explicitly into USB/SIDE/FACE with signs.

## Other physical evidence from the same run

The normal ES-002 runtime successfully detected QMI8658 at `0x6B`, WHO_AM_I `0x05`, revision `0x7C`, and sustained approximately 180 Hz successful polling with zero read failures during the captured run. BOOT short and long events were also observed through the configured GPIO0 active-low input path.

Do not infer a sustained-safe LED brightness from this calibration session.
