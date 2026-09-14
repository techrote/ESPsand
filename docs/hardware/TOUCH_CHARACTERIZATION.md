# ES-003 no-component capacitive characterization

This document defines the physical gate for ES-003. The firmware and normalization logic are automated and host-tested; actual sensitivity and false-positive behaviour can only be judged on the owner's Waveshare ESP32-S3-Matrix.

## Hardware truth and candidates

Waveshare's official schematic for the ESP32-S3-Matrix routes GPIO1 through GPIO7 directly to the exposed expansion header. All seven are native ESP32-S3 touch-capable GPIOs and none is assigned to the onboard matrix, IMU, BOOT button or native USB path.

Official schematic:
`https://files.waveshare.com/wiki/ESP32-S3-Matrix/ESP32-S3-Matrix-Sch.pdf`

The characterization set is therefore exactly:

```text
GPIO1 GPIO2 GPIO3 GPIO4 GPIO5 GPIO6 GPIO7
```

GPIO8 and GPIO9 are touch-capable in the ESP32-S3 silicon but are not exposed on this board's edge header, so they are not useful no-component electrodes here. GPIO10 through GPIO14 are excluded because the board uses them for the QMI8658 and RGB matrix. GPIO19/GPIO20 are native USB and are excluded.

Routing/capability is known. **Useful capacitive sensitivity is not yet known.** Do not promote a channel or virtual zone to product input from the schematic alone.

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

Baseline tracking is fast only during initial warmup, slow while idle, and nearly frozen during a strong disturbance. Equal changes across all channels are removed from each channel's local `z` score and retained separately as a common-mode value. Event gates add hysteresis and cooldown.

Two virtual groupings exist only to accelerate characterization:

- provisional A: GPIO5, GPIO6, GPIO7;
- centre probe: GPIO4, not assigned to either group;
- provisional B: GPIO1, GPIO2, GPIO3.

These groupings are **not enabled as scene controls**. `TouchFrame.available` remains false until physical evidence demonstrates a useful mapping. The `NullTouchZones` adapter provides the permanent clean unavailable/disabled path.

## Diagnostic page

The normal firmware now cycles through four diagnostic pages:

1. pixel sweep;
2. primary colours;
3. gravity;
4. touch characterization.

Long-press BOOT three times from the initial pixel-sweep page to reach touch characterization.

On the matrix, columns 0 through 6 correspond to GPIO1 through GPIO7. A cyan bar grows upward with positive common-mode-corrected disturbance and turns amber when the provisional channel threshold is active. Column 7 is the positive common-mode disturbance in purple. During initial warmup the seven channels show only dim bottom markers.

While the touch page is active, serial emits approximately 10 lines/s. Example shape:

```text
touch t_ms=... hw=1 ready=1 zones=0 scans=... cm=... pa=.../0 pb=.../0 pc=.../0 ch1:r... b... d... n... zr... z... a0 ... ch7:...
```

Fields:

- `hw`: touch hardware/candidate validation succeeded;
- `ready`: adaptive warmup has completed;
- `zones`: product semantic zones enabled; expected to remain `0` during characterization;
- `scans`: processed complete seven-channel scans;
- `cm`: normalized common-mode disturbance before local cancellation;
- `pa`, `pb`, `pc`: provisional A/B/combo intensities and active state;
- `r`: raw reading;
- `b`: adaptive baseline;
- `d`: raw minus baseline;
- `n`: adaptive noise magnitude;
- `zr`: raw normalized disturbance;
- `z`: common-mode-corrected normalized disturbance;
- `a`: per-channel provisional active state.

## Exact bare-board test procedure

No foil, wire, resistor, external touch IC or added electrode is permitted for this issue. Use only a finger near or on the exposed numbered GPIO pads. Avoid the 5 V, 3V3 and GND pads during this test.

1. Start with the board untouched and dangling or resting in a repeatable nonconductive position. Power it from USB and allow at least 3 seconds for startup/warmup.
2. Enter the touch-characterization page with long BOOT presses. Confirm serial reports `hw=1`, `ready=1`, `zones=0`.
3. **Idle capture:** keep hands at least roughly 10 cm from the GPIO edge for 10 seconds. Save the serial output. This establishes idle noise and drift.
4. **Individual direct-touch pass:** for GPIO1 through GPIO7 in order, touch only that numbered exposed pad for about 2 seconds, release for about 2 seconds, then move to the next pad. Do not intentionally touch neighbouring pads at the same time.
5. **Individual proximity pass:** repeat GPIO1 through GPIO7 without contact, bringing one fingertip approximately 1-5 mm from each pad for about 2 seconds. Exact distance is not critical; note whether any response is visibly/serially obvious before contact.
6. **Broad-zone pass:** bring a finger or thumb broadly near GPIO1-3 together, then GPIO5-7 together, each for about 2 seconds with 2 seconds release. This tests whether the physical edge can behave as two coarse regions despite having no added electrode.
7. **Combo/common-mode pass:** disturb both ends of the GPIO1-7 edge together, for example two fingers near the GPIO1-3 and GPIO5-7 regions simultaneously. Record whether `cm`/`pc` separates this from a local disturbance.
8. **False-positive pass:** leave the GPIO edge untouched while tilting, rotating and gently shaking the board for about 10 seconds. Then handle the USB connector/cable normally for another 10 seconds. Capacitive sensing must not become a scene-changing control due to ordinary motion or USB handling.
9. Return to idle for 10 seconds and confirm baselines recover instead of remaining latched.

For the quickest useful review, paste the serial sections for: idle, one strong low-numbered channel event, one strong high-numbered channel event, broad A, broad B, combo, and motion-without-touch. A full capture is also acceptable.

## Physical decision criteria

ES-003 does not require touchscreen-grade behaviour. A channel/group is useful when repeated deliberate disturbance is comfortably larger than its idle variation and ordinary board handling does not trigger it continuously.

Possible successful conclusions are:

- **two zones viable:** two spatially distinct broad disturbances can be separated with tolerable false positives;
- **one zone viable:** only a generic broad capacitive disturbance is robust; expose one semantic intensity rather than pretending there are two;
- **combo-only/common-mode useful:** local discrimination is poor but a deliberate large hand-near-board event is distinctive enough for a bounded special-event input;
- **not viable bare-board:** deliberate response is not reliably separable from noise/handling. Keep `ITouchZones` unavailable and continue ESPsand using BOOT + IMU.

Any of these outcomes satisfies the experimental intent if it is supported by captured evidence. No scene may depend on capacitive success.

## Coating note

Future conformal coating changes electrode-to-finger geometry and dielectric properties. The adaptive baseline avoids factory absolute thresholds, but coating still requires repeating physical characterization and may change which virtual grouping is useful.
