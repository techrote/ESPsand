# ESPsand v0 input model

## Philosophy

ESPsand has almost no conventional UI. Physical handling is the UI. Hardware conditioning produces a small set of normalized semantic values/events before the deterministic model boundary.

Once normalized into an `InputFrame`, a recorded input sequence is ordinary deterministic simulation data. Raw sensor axes, hardware polling jitter and touch-electrical irregularity are not model entropy.

## BOOT button

Product semantics remain:

- short press: reset/reseed the current scene according to runtime scene policy;
- long press: advance to the next scene;
- ambiguous duration between thresholds: do not trigger both.

The simulation-side `BootEvent` remains available in `InputFrame`, but generic `Model::step()` does not silently perform lifecycle transitions. `SceneRuntime` owns product lifecycle calls.

ES-007 has one product scene, so short BOOT restores the exact configured Lava + Water seed/initial state. Long BOOT currently performs a one-scene wrap by reseeding Lava + Water with seed+1 and logs that fact. Once ES-008 adds Scene 2, long BOOT should become real scene advance rather than preserving this temporary wrap behavior.

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

A missing/invalid sample decays transient motion fields rather than replacing the stored gravity direction with garbage. Before the first valid IMU sample, `SceneRuntime` supplies deterministic downward gravity so Lava + Water remains autonomous and visible.

The important contract is separation: a shake may increase transport disturbance and scene fracture response, but it does not become a new low-frequency gravity direction. Host tests explicitly cover this.

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

ES-007 layers only scene-specific, bounded interpretation on top:

- sufficiently strong shake/tap/motion/noise may relocate a small capped number of existing crust cells, preserving their mass and reopening contact surfaces;
- this is scene policy around the shared dynamics engine, not a second transport solver.

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

Independent A/B touch buttons were not physically supported by the bare board and must not be silently reintroduced by scene documentation or code.

### ES-007 Lava + Water mapping

When optional touch is available:

- active slider with strength >=0.35 can inject one lava cell near the selected horizontal position, at a maximum cadence of once per 12 model ticks and subject to event budget;
- a combo edge event requests one adjacent lava/water contact pair in available interior space; the ordinary ES-006 reaction engine then performs crust/steam conversion under the reaction budget;
- explicit noise contributes only to the same bounded disturbance interpretation used for remixing; it is not PRNG seed material.

The scene remains fully playable/observable without touch because autonomous material replenishment plus BOOT + IMU are sufficient.

### External noise is not randomness

`noise_impulse` is explicit external input, not hidden entropy. Live hardware irregularity becomes replayable only when represented in the ordered `InputFrame` sequence. Scene stochastic placement uses only the model-owned PCG32.

### Characterization mode

The separate diagnostic/bring-up code still exposes raw/baseline/noise/local/common-mode/slider/noise-event information for hardware research. Product scene code consumes semantic `TouchFrame` fields only.

## Event injection and replay

Scenes consume exactly one normalized `InputFrame` per executed model tick. Replay identity is seed/configuration + reset state + tick count + ordered `InputFrame` sequence, not wall-clock timing.

ES-007 runs a scripted 96-tick duplicate-model trace containing four gravity directions plus shake, tap, combo and slider input and requires equal state hashes after every tick. A separate 1,200-tick randomized duplicate run checks the same replay/boundedness property over a larger scene history.
