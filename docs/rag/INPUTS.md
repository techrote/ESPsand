# ESPsand v0 input model

## Philosophy

ESPsand has almost no conventional UI. Physical handling is the UI. Input interpretation should therefore produce a small set of semantic events rather than leaking noisy raw hardware values into scenes.

## BOOT button

Required semantics:

- short press: reset/reseed the current scene;
- long press: advance to next scene;
- ambiguous duration between thresholds: resolve conservatively; do not trigger both.

Planning defaults may use ~600 ms maximum short and ~800 ms minimum long, but implementation should tune against the actual button/debounce behaviour.

A long press should not also issue a short-reset event on release.

## Gravity from accelerometer

Use a low-pass estimate of the gravity/specific-force direction while the board is under ordinary handheld motion. Transform it into matrix/world coordinates using an explicit board-axis calibration.

The scene/model consumes a normalized 2D gravity vector plus confidence/magnitude metadata, not raw sensor axes.

When total acceleration is far from the expected static magnitude, avoid treating the instantaneous vector as a perfect gravity direction. Preserve the previous low-frequency estimate and separately emit disturbance energy.

## Motion events

Derive at least:

- `shake_energy`: sustained high-frequency/transient acceleration;
- `tap_impulse`: short high-amplitude impulse candidate;
- `spin_rate`: gyro magnitude or useful projected rotation;
- `motion_energy`: smoothed general agitation.

Thresholds need hysteresis and cooldowns so one physical tap does not generate dozens of events.

Scenes may interpret these differently. Examples:

- lava/water: fracture crust or produce violent slosh;
- oil/fire: redistribute fuel/embers;
- moss/mites: scatter agents/seeds;
- tracer plume: mix/stir scalar concentration.

## Capacitive zones — tested board result

The original goal was to obtain one or two broad interaction regions with **no added components** by exploiting unused native touch-capable ESP32-S3 GPIO pads/traces.

ES-003 physically tested GPIO1..GPIO7 on the Waveshare ESP32-S3-Matrix. The bare-board result is **combo-only/common-mode useful**:

- swiping along the exposed GPIO1..7 edge produces a smooth local-response motion in diagnostics;
- a fingertip spans most or all of the edge, so individual-pad or clean two-zone operation is not reliable;
- pinching along the PCB edge gives the strongest repeatable deliberate gesture;
- common-mode magnitude tracks broad fingertip/PCB contact area well enough to provide a bounded intensity;
- local channels can flash spuriously, so local A/B activity is not exposed as product input.

Therefore the accepted bare-board semantics are:

- `cap_a = 0`, `event_a = false`;
- `cap_b = 0`, `event_b = false`;
- `cap_combo` = normalized common-mode edge-contact intensity;
- `event_combo` = hysteretic/cooldown-gated deliberate edge-contact event;
- baseline drift alone -> no event.

This is not pressure sensing. Coupling can vary with contact area, grip, moisture, grounding and other environmental factors.

False positives are acceptable only when bounded. Local diagnostic false activity must not change scenes, corrupt state, lock the runtime or create sustained maximum brightness. The product combo event path therefore uses common mode directly rather than local per-channel activity.

### Characterization mode

Firmware provides a diagnostic mode/serial stream showing, per candidate touch channel:

- raw reading;
- adaptive baseline;
- delta/normalized delta;
- estimated noise;
- event state;
- common-mode estimate.

Local GPIO1..GPIO7 diagnostics remain useful for research and future coating/recalibration even though the current product mapping is combo-only.

### Normalization

Use normalized disturbance relative to measured noise/baseline with clipping and adaptive calibration. On ESP32-S3, touch raises the raw capacitive reading.

Adapt baseline slowly when idle; freeze or slow baseline adaptation during strong touch events. Common-mode and local-channel components remain separated so noisy local bars cannot directly trigger the accepted combo semantic.

### Graceful fallback

`ITouchZones` reports unavailable/disabled cleanly when hardware initialization fails. Every scene remains fully usable with button + IMU only, and no scene may depend on capacitive success.

## Event injection

Scenes consume a per-tick `InputFrame` or equivalent containing normalized continuous fields and edge-triggered events. Tests should be able to construct this structure without hardware.
