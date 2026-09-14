# ES-003 no-component capacitive characterization

This document records the ES-003 bare-board capacitive experiment on the owner's Waveshare ESP32-S3-Matrix. The firmware and normalization logic are automated and host-tested; the sensitivity conclusions below come from direct physical use of the target board.

## Hardware truth and candidates

Waveshare's official schematic routes GPIO1 through GPIO7 directly to the exposed expansion header. All seven are native ESP32-S3 touch-capable GPIOs and none is assigned to the onboard matrix, IMU, BOOT button or native USB path.

Official schematic:
`https://files.waveshare.com/wiki/ESP32-S3-Matrix/ESP32-S3-Matrix-Sch.pdf`

The characterization set is therefore exactly:

```text
GPIO1 GPIO2 GPIO3 GPIO4 GPIO5 GPIO6 GPIO7
```

GPIO8 and GPIO9 are touch-capable in the ESP32-S3 silicon but are not exposed on this board's edge header. GPIO10 through GPIO14 are excluded because the board uses them for the QMI8658 and RGB matrix. GPIO19/GPIO20 are native USB and are excluded.

## Firmware behaviour

Arduino-ESP32 2.0.17 provides `touchRead()` on ESP32-S3. On S2/S3, a larger raw value represents increased touch capacitance. The board adapter pre-initializes all seven channels during startup, then reads one channel at a time on a bounded schedule. A complete seven-channel scan is processed approximately every 28 ms.

The pure normalization layer maintains, per channel:

- raw reading;
- adaptive baseline;
- signed delta from baseline;
- adaptive noise magnitude with a floor;
- raw normalized disturbance `zr`;
- common-mode-corrected disturbance `z`;
- hysteretic active state.

Baseline tracking is fast only during initial warmup, slow while idle, and nearly frozen during a strong disturbance. Equal changes across all channels are removed from each local `z` score and retained separately as a common-mode value. Event gates add hysteresis and cooldown.

Two three-pin virtual groups remain available for diagnostics:

- provisional A: GPIO5, GPIO6, GPIO7;
- centre probe: GPIO4;
- provisional B: GPIO1, GPIO2, GPIO3.

They are **not product A/B controls** on the tested bare board. Physical characterization showed that a fingertip spans most or all of the exposed GPIO1..7 edge, so individual-pad or two-zone semantics would imply precision the hardware does not reliably provide.

## Physical result

With the touch-characterization page active, the owner physically observed:

- swiping a fingertip along the GPIO1..7 edge moves the local-response amber/yellow indication smoothly from left to right;
- an ordinary fingertip is wide enough to influence most or all of the numbered edge at once, so reliably isolating an individual pad is impractical;
- the strongest and most repeatable response occurs when **pinching along the PCB edge** rather than attempting point contact on one numbered pad;
- the purple common-mode bar is strong during that edge pinch and visibly tracks the amount of fingertip area coupled to the PCB;
- local common-mode-rejected channels occasionally flash amber spuriously, so their apparent position is useful diagnostically but is not reliable enough to expose as scene-changing A/B input.

This is a successful **combo-only/common-mode** outcome. The bare board provides a useful broad deliberate-contact gesture, but not trustworthy separate A/B buttons.

The product mapping is therefore:

- `cap_a = 0`, `event_a = false` on this board profile;
- `cap_b = 0`, `event_b = false` on this board profile;
- `cap_combo` is the bounded common-mode edge-contact intensity;
- `event_combo` is the hysteretic/cooldown-gated deliberate edge-contact event;
- local per-channel values remain available in diagnostics for research and possible future coated-board recalibration.

Importantly, `cap_combo` is driven from common mode only. Occasional local amber false activity cannot directly become a product capacitive event.

## Diagnostic page

The normal firmware cycles through four diagnostic pages:

1. pixel sweep;
2. primary colours;
3. gravity;
4. touch characterization.

Long-press BOOT three times from the initial pixel-sweep page to reach touch characterization.

On the matrix, columns 0 through 6 correspond to GPIO1 through GPIO7. A cyan bar grows upward with positive common-mode-corrected disturbance and turns amber when the provisional local-channel threshold is active. Column 7 is the positive common-mode disturbance in purple. During initial warmup the seven channels show only dim bottom markers.

While the touch page is active, serial emits approximately 10 lines/s. After this physical decision, `zones=1` means a supported capacitive semantic input exists; it does **not** mean two local zones exist. The supported semantic is combo/common-mode only.

Example shape:

```text
touch t_ms=... hw=1 ready=1 zones=1 scans=... cm=... pa=... pb=... pc=... ch1:... ch7:...
```

Fields:

- `hw`: touch hardware/candidate validation succeeded;
- `ready`: adaptive warmup has completed;
- `zones`: at least one product capacitive semantic is enabled;
- `scans`: processed complete seven-channel scans;
- `cm`: normalized common-mode disturbance before local cancellation;
- `pa`, `pb`: diagnostic-only provisional local-group intensities;
- `pc`: combo/common-mode intensity and active state;
- `r`: raw reading;
- `b`: adaptive baseline;
- `d`: raw minus baseline;
- `n`: adaptive noise magnitude;
- `zr`: raw normalized disturbance;
- `z`: common-mode-corrected local disturbance;
- `a`: per-channel diagnostic active state.

## Final ES-003 decision

The tested board satisfies ES-003 as **combo-only/common-mode useful**:

- no extra electrode or component is required;
- a deliberate edge pinch gives a strong, smooth response;
- response magnitude carries useful broad contact-area/coupling information;
- local channel position is visually interesting but too coarse/noisy to promise individual-pad or two-zone operation;
- false local amber activity is isolated from the product combo event path;
- BOOT + IMU remain fully sufficient when capacitive input is ignored.

This is intentionally not described as pressure sensing. Coupling depends on contact area, grip, moisture, grounding and other environmental factors as well as force.

## Coating note

Future conformal coating changes electrode-to-finger geometry and dielectric properties. The adaptive baseline avoids factory absolute thresholds, but coating still requires repeating physical characterization. A coated board may make local-zone discrimination better or worse, so the current combo-only decision applies to the tested bare-board geometry.
