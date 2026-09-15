# ESPsand v0 input model

## Philosophy

ESPsand has almost no conventional UI. Physical handling is the UI. Hardware conditioning produces a small set of normalized semantic values/events before the deterministic model boundary.

Once normalized into an `InputFrame`, a recorded input sequence is ordinary deterministic simulation data. Raw sensor axes, hardware polling jitter and touch-electrical irregularity are not model entropy.

## BOOT button — ES-008 product lifecycle

Product semantics are now concrete across three scenes:

- short press: reset the current scene exactly at its current configured seed;
- long press: advance one entry in the stable product catalogue;
- ambiguous duration between thresholds: do not trigger both.

The current order is:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Lava + Water
```

Long BOOT increments the deterministic seed before initializing the next scene. The simulation-side `BootEvent` remains available in `InputFrame`, but generic `Model::step()` does not silently perform lifecycle transitions; `SceneRuntime` owns product lifecycle calls.

## Gravity and motion — ES-007 concrete conditioner

`MotionInterpreter` is the pure board-independent conditioner between `ImuSample` and `InputFrame`.

For each valid sample it:

- sanitizes acceleration and gyro components;
- maintains a low-pass 3D gravity estimate;
- derives confidence from how close instantaneous acceleration magnitude is to 1g;
- reduces low-pass following during low-confidence/high-transient motion;
- projects the gravity estimate through the explicit board-to-matrix transform;
- emits bounded high-frequency `shake_energy` separately from gravity;
- emits smoothed bounded `motion_energy` from acceleration residual and gyro activity;
- emits a bounded `tap_impulse` for sufficiently strong residual acceleration, with a 160 ms cooldown;
- emits signed normalized `spin_rate` from gyro Z.

A missing/invalid sample decays transient motion fields rather than replacing the stored gravity direction with garbage. Before the first valid IMU sample, `SceneRuntime` supplies deterministic downward gravity so autonomous scenes remain visible.

The important contract is separation: a shake may increase transport disturbance and scene response, but it does not become a new low-frequency gravity direction. Host tests explicitly cover this.

## `InputFrame` contract

`espsand::sim::InputFrame` is constructed once per logical tick and contains:

- normalized 2D gravity, gravity magnitude and confidence;
- shake, motion, tap and signed spin disturbance values;
- BOOT-derived lifecycle intent;
- `cap_combo` plus its edge event;
- coarse pinch-slider active/position/strength;
- explicit external `noise_impulse` plus its event.

Continuous scalar fields are sanitized at the model boundary. Gravity/spin clamp to -1..1; unit-like values clamp to 0..1; non-finite values become neutral defaults.

ES-006 gives the motion fields shared transport meaning: gravity chooses transport direction while shake/motion/tap/absolute spin provide a bounded mobility boost and signed spin biases lateral relaxation.

ES-007 adds one scene-specific motion rule: sufficiently strong disturbance may relocate a small capped number of existing crust cells in Lava + Water, preserving their mass and reopening contact surfaces. ES-008 does not add another transport solver: Sodium-like + Water and Oil + Fire use the common disturbance/mobility behavior directly.

## Capacitive edge input — tested board truth

ES-003/003A physically tested GPIO1..GPIO7 on the Waveshare ESP32-S3-Matrix. The accepted bare-board semantics remain deliberately coarse:

- `cap_a = 0`, `event_a = false`;
- `cap_b = 0`, `event_b = false`;
- `cap_combo` = normalized common-mode edge-contact intensity;
- `event_combo` = hysteretic/cooldown-gated deliberate broad-contact event;
- `slider_active` requires strong common mode and at least two active local channels;
- `slider_position` is the smoothed 0..1 weighted centroid of positive GPIO1..GPIO7 local response;
- `slider_strength` is bounded broad-contact strength;
- a strong isolated local excursion may produce explicit bounded `noise_impulse` / `noise_event`.

Independent A/B touch buttons were not physically supported by the bare board and are not exposed by any current product scene.

### Product scene mappings

All optional direct-control injection is capped to no more than once per 12 model ticks and participates in the event budget.

**Lava + Water**

- slider: inject one lava cell near the selected X;
- combo: insert one adjacent lava/water contact pair; the shared reaction engine performs crust/steam conversion;
- strong disturbance/noise may contribute to the bounded crust-remix rule.

**Sodium-like + Water — ES-008**

- slider: inject one sodium-like cell near the selected X;
- combo: insert one adjacent sodium-like/water pair; the shared reaction engine produces finite fire/steam and the ordinary bounded reaction impulse;
- shake/tap/motion use the shared mobility/disturbance path to increase encounters rather than triggering a scene-local reaction cascade.

**Oil + Fire — ES-008**

- slider: inject one oil cell near the selected X;
- combo: ignite one existing oil cell; subsequent propagation uses the centralized oil/fire reaction and generic finite fire lifecycle;
- shake/tap/motion use the shared mobility/disturbance path to rearrange fuel/fire/smoke.

All three scenes remain fully demonstrable with BOOT + IMU if touch is unavailable.

### External noise is not randomness

`noise_impulse` is explicit external input, not hidden entropy. Live hardware irregularity becomes replayable only when represented in the ordered `InputFrame` sequence. Scene stochastic placement uses only the model-owned PCG32.

### Characterization mode

The separate diagnostic/bring-up code still exposes raw/baseline/noise/local/common-mode/slider/noise-event information for hardware research. Product scene code consumes semantic `TouchFrame` fields only.

## Event injection and replay

Scenes consume exactly one normalized `InputFrame` per executed model tick. Replay identity is seed/configuration + reset state + tick count + ordered `InputFrame` sequence, not wall-clock timing.

ES-007 retains its scripted 96-tick Lava + Water duplicate-model trace. ES-008 adds fixed-seed duplicate traces for Sodium-like + Water and Oil + Fire plus randomized 1,000-tick duplicate runs for each new scene, checking state-hash equality, invariant preservation and budget bounds throughout.
