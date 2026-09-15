# ESPsand v0 input model

## Philosophy

ESPsand has almost no conventional UI. Physical handling is the UI. Input interpretation should therefore produce a small set of semantic values/events rather than leaking noisy raw hardware values into scenes.

Hardware conditioning and sampling cadence live outside the deterministic model. Once normalized into an `InputFrame`, a recorded input sequence is ordinary deterministic simulation data.

## BOOT button

Required product semantics:

- short press: reset/reseed the current scene according to runtime scene policy;
- long press: advance to the next scene;
- ambiguous duration between thresholds: resolve conservatively; do not trigger both.

A long press must not also issue a short-reset event on release.

The simulation-side `BootEvent` is `kNone`, `kResetScene` or `kNextScene`. ES-004 carries that semantic event in `InputFrame`; lifecycle policy remains with the runtime/scene controller, which calls the model's explicit reset/reseed/scene-selection seam rather than making generic `Model::step()` silently change lifecycle.

## Gravity from accelerometer

Use a low-pass estimate of the gravity/specific-force direction while the board is under ordinary handheld motion. Transform it into matrix/world coordinates using the explicit board-axis calibration.

The model consumes a normalized 2D gravity vector plus magnitude/confidence metadata, not raw sensor axes.

When total acceleration is far from the expected static magnitude, avoid treating the instantaneous vector as a perfect gravity direction. Preserve the previous low-frequency estimate and separately emit disturbance energy.

## Motion events

The simulation-side contract provides:

- `shake_energy`: sustained high-frequency/transient acceleration;
- `tap_impulse`: short high-amplitude impulse candidate;
- `spin_rate`: normalized useful projected rotation;
- `motion_energy`: smoothed general agitation.

Hardware/input conditioning owns thresholds, hysteresis and cooldowns so one physical action does not emit an unbounded event stream.

ES-006 gives these fields shared mechanics meaning without coupling them to raw hardware. Low-frequency gravity chooses the transport direction. `shake_energy`, `motion_energy`, `tap_impulse` and absolute spin only raise a bounded transport-disturbance/mobility term; they do not overwrite the gravity vector. Signed `spin_rate` biases deterministic lateral-relaxation direction. This separation is host-tested so a shake cannot silently become a new gravity direction.

Hero scenes may later layer additional bounded semantics such as crust fracture or injection on top of the same normalized fields.

## `InputFrame` contract

`espsand::sim::InputFrame` is a hardware-independent value constructed once per logical tick. It contains:

- normalized 2D gravity, gravity magnitude and confidence;
- shake, motion, tap and spin disturbance values;
- BOOT-derived semantic lifecycle event;
- `cap_combo` plus its edge event;
- coarse pinch-slider active/position/strength;
- explicit external `noise_impulse` plus its event.

Continuous normalized scalar fields are sanitized to bounded ranges at the model boundary. Gravity components and signed spin are clamped to -1..1; the other continuous fields are clamped to 0..1. Non-finite values are replaced by neutral defaults, and an invalid enum value becomes `kNone`. This is a defensive replay boundary, not hardware filtering.

The runtime may evolve its raw-IMU conditioning independently as long as it produces this normalized contract. ES-006 deliberately consumes only this contract. Sensor sample timing, raw acceleration units and raw gyro values are not visible to `DynamicsEngine`.

## Capacitive edge input — tested board result

ES-003/003A physically tested GPIO1..GPIO7 on the Waveshare ESP32-S3-Matrix. The bare-board result is useful but deliberately coarse:

- swiping along the exposed GPIO1..GPIO7 edge produces a smooth local-response motion in diagnostics;
- a fingertip spans most or all of the edge, so individual-pad or clean two-zone operation is not reliable;
- pinching along the PCB edge gives the strongest repeatable deliberate gesture;
- common-mode magnitude tracks broad fingertip/PCB contact area well enough to provide a bounded intensity;
- local channels can flash spuriously, so local A/B activity is not exposed as button-like product input.

The accepted bare-board semantics are:

- `cap_a = 0`, `event_a = false`;
- `cap_b = 0`, `event_b = false`;
- `cap_combo` = normalized common-mode edge-contact intensity;
- `event_combo` = hysteretic/cooldown-gated deliberate edge-contact event;
- `slider_active` only when common mode is near diagnostic full scale **and at least two local channels are simultaneously active**;
- `slider_position` = smoothed 0..1 weighted centroid of positive GPIO1..GPIO7 local response while the slider gate is active;
- `slider_strength` = bounded broad-contact strength;
- a strong isolated single-channel near-full excursion may produce an explicit bounded `noise_impulse`/`noise_event` input candidate.

The slider is intentionally a coarse direct control rather than a precision touch UI. A later scene may map it to spawn rate, reaction bias, injection position or another bounded parameter. Scene meaning is not fixed by the input layer.

### External noise is not randomness

`noise_impulse` is an explicit external input and is **not** seed material, PRNG state or a hidden entropy path. If a live run observes hardware irregularity, that value is recorded in the tick's `InputFrame`. Replaying the same frames reproduces the same result.

The ES-004 determinism fixture explicitly verifies that cap/noise fields can alter modeled fixture state without changing PCG32 state. ES-006 likewise introduces no hardware-entropy path: its transport mobility schedule is deterministic from tick/cell state, and any future PRNG draw must come from the model-owned PCG32 through an explicit rule.

This is not pressure sensing. Coupling can vary with contact area, grip, moisture, grounding and other environmental factors.

False positives are acceptable only when bounded. Local diagnostic false activity must not change scenes, corrupt state, lock the runtime or create sustained maximum brightness.

### Characterization mode

Firmware provides a diagnostic mode/serial stream showing, per candidate touch channel:

- raw reading;
- adaptive baseline;
- delta/normalized delta;
- estimated noise;
- local active state;
- common-mode estimate;
- slider active/position/strength;
- isolated noise impulse/event.

Local GPIO1..GPIO7 diagnostics remain useful for research and future coating/recalibration even though A/B buttons are disabled.

### Normalization

Touch hardware conditioning uses normalized disturbance relative to measured noise/baseline with clipping and adaptive calibration. On ESP32-S3, touch raises the raw capacitive reading.

Baseline adapts slowly when idle and is frozen/slowed during strong touch. Common-mode and local-channel components remain separated so noisy local bars cannot directly trigger the accepted combo semantic.

### Graceful fallback

`ITouchZones` reports unavailable/disabled cleanly when hardware initialization fails. Every scene remains fully usable with button + IMU only, and no scene may depend on capacitive success.

## Event injection and replay

Scenes consume one normalized `InputFrame` per fixed model tick. Host tests can construct these frames directly without hardware. Replay identity is defined by seed/configuration, reset/initial state, tick count and the ordered `InputFrame` sequence—not by wall-clock timing or sensor polling jitter.

ES-006 tests repeated multi-axis gravity/shake/tap/spin sequences through the full `Model::step()` path and requires byte-identical state-hash replay between duplicate models.
