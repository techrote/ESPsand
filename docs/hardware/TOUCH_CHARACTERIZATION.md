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
- local common-mode-rejected channels occasionally flash amber spuriously, often around the same time as correlated cyan/blue subthreshold motion;
- occasional very fast single-column near-full excursions are present and are useful as an optional source of external live disturbance rather than as a precision control.

The bare board therefore provides useful broad contact plus a coarse position signal during a strong pinch, but not trustworthy separate A/B buttons.

## Accepted product mapping

- `cap_a = 0`, `event_a = false` on this board profile;
- `cap_b = 0`, `event_b = false` on this board profile;
- `cap_combo` is the bounded common-mode edge-contact intensity;
- `event_combo` is the hysteretic/cooldown-gated deliberate edge-contact event;
- a **coarse slider** becomes valid only when the common-mode signal is near diagnostic full scale and at least two local channels are simultaneously above their active threshold;
- while valid, `slider_position` is the smoothed weighted centroid of positive local GPIO1..7 response, mapped left-to-right into 0..1;
- `slider_strength` records the broad-contact strength;
- a strong isolated single-channel near-full excursion may become a bounded `noise_impulse` and edge-triggered `noise_event`.

This gating intentionally matches the physical observation: the yellow local bars are trusted as direct position only while the purple broad-contact bar says a deliberate pinch is definitely happening. Random local amber flashes therefore do not become a slider by themselves.

The isolated noise path is not PRNG state. It is an explicit external input. If a future scene uses it, deterministic replay records the impulse in that tick's `InputFrame`; the model remains deterministic for an identical input trace.

## What the cyan/blue pixels mean

On the matrix, columns 0 through 6 correspond to GPIO1 through GPIO7. The cyan/blue height is the positive **common-mode-corrected local normalized disturbance** `z`. It is real sensor telemetry, not decorative noise:

- one or two pixels along the bottom usually mean small positive subthreshold variation;
- several neighbouring columns rising together means a correlated but non-uniform capacitive disturbance remains after common-mode cancellation;
- a column turns amber/yellow only after its local hysteretic threshold is crossed;
- column 7 is purple and represents positive common-mode disturbance before local cancellation.

The common-mode calculation is the mean normalized raw disturbance across all sampled channels. Local `z` subtracts most (currently 75%) of that common component. A broad but uneven physical or environmental disturbance can therefore move several cyan bars together while one more-sensitive column crosses into amber. The observed tendency for column 6 to flash amber while neighbouring cyan bars move is consistent with that signal path; it is not evidence of a separate hidden input.

## Diagnostic page

The normal firmware cycles through four diagnostic pages:

1. pixel sweep;
2. primary colours;
3. gravity;
4. touch characterization.

Long-press BOOT three times from the initial pixel-sweep page to reach touch characterization.

During initial warmup the seven local channels show only dim bottom markers. After warmup, cyan bars show subthreshold local response, amber shows local threshold activity, and purple shows common mode.

While the touch page is active, serial emits approximately 10 lines/s. Example shape:

```text
touch t_ms=... hw=1 ready=1 zones=1 scans=... cm=... pa=... pb=... pc=... slider=1/0.42/1.00 noise=0.00/0 ch1:... ch7:...
```

Fields:

- `hw`: touch hardware/candidate validation succeeded;
- `ready`: adaptive warmup has completed;
- `zones`: at least one product capacitive semantic is enabled;
- `scans`: processed complete seven-channel scans;
- `cm`: normalized common-mode disturbance before local cancellation;
- `pa`, `pb`: diagnostic-only provisional local-group intensities;
- `pc`: combo/common-mode intensity and active state;
- `slider`: active / position / broad-contact strength;
- `noise`: isolated-noise impulse / edge-event state;
- `r`: raw reading;
- `b`: adaptive baseline;
- `d`: raw minus baseline;
- `n`: adaptive noise magnitude;
- `zr`: raw normalized disturbance;
- `z`: common-mode-corrected local disturbance;
- `a`: per-channel diagnostic active state.

## Final capability decision

The tested bare board supports:

- deliberate edge-pinch common-mode input;
- coarse direct position during a sufficiently strong multi-channel pinch;
- explicit bounded live irregularity from isolated fast local excursions;
- diagnostic local-channel visualization;
- no reliable individual-pad or two-button promise.

This is intentionally not described as pressure sensing. Coupling depends on contact area, grip, moisture, grounding and other environmental factors as well as force.

## Coating note

Future conformal coating changes electrode-to-finger geometry and dielectric properties. The adaptive baseline avoids factory absolute thresholds, but coating still requires repeating physical characterization. A coated board may make local-zone discrimination better or worse, so the current semantics apply to the tested bare-board geometry.
