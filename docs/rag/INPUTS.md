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

## Capacitive zones — experimental

Goal: obtain one or two broad interaction regions with **no added components** by exploiting unused native touch-capable ESP32-S3 GPIO pads/traces if the actual board permits.

This is not a precision touch UI. Desired semantics:

- significant relative disturbance near region A -> `cap_a` event/intensity;
- significant relative disturbance near region B -> `cap_b` event/intensity;
- large common/ambiguous disturbance -> `cap_combo` or probabilistic special event;
- baseline drift alone -> no event.

False positives are acceptable if they produce interesting, bounded behaviour. False positives must not change scenes, corrupt state, lock the runtime or create sustained maximum brightness.

### Characterization mode

Firmware should provide a diagnostic mode/serial stream showing, per candidate touch channel:

- raw reading;
- adaptive baseline;
- delta/normalized delta;
- estimated noise;
- event state;
- common-mode estimate when multiple channels are sampled.

Test candidate free touch GPIOs rather than assuming a particular pair.

### Normalization

Prefer normalized disturbance relative to measured noise/baseline, e.g. conceptual `z = (baseline - sample) / noise_scale`, with clipping and adaptive calibration. Exact sign depends on the driver/readout.

Adapt baseline slowly when idle; freeze or slow baseline adaptation during strong touch events.

### Graceful fallback

`ITouchZones` must report unavailable/disabled cleanly. Every scene remains usable with button + IMU only.

## Event injection

Scenes consume a per-tick `InputFrame` or equivalent containing normalized continuous fields and edge-triggered events. Tests should be able to construct this structure without hardware.
