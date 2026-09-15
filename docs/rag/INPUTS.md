# ESPsand v0 input model

## Philosophy

ESPsand has almost no conventional UI. Physical handling is the UI. Hardware conditioning produces normalized semantic values/events before the deterministic model boundary.

Once normalized into an `InputFrame`, a recorded input sequence is ordinary deterministic simulation data. Raw sensor axes, polling jitter and touch-electrical irregularity are not model entropy.

## BOOT button — ES-009 product lifecycle

Product semantics are now concrete across four scenes:

- short press: reset the current scene exactly at its current configured seed;
- long press: advance one entry in the stable product catalogue and increment the deterministic seed;
- ambiguous duration between thresholds: trigger neither short nor long behavior.

Current order:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

The simulation-side `BootEvent` remains available in `InputFrame`, but generic `Model::step()` does not perform product lifecycle transitions; `SceneRuntime` owns reset/scene advance.

## Gravity and motion

`MotionInterpreter` remains the pure board-independent conditioner between `ImuSample` and `InputFrame`.

It sanitizes acceleration/gyro, maintains low-pass gravity, projects gravity through the explicit board-to-matrix transform, and emits bounded shake/motion/tap/spin separately. Missing samples decay transient fields. Before the first valid IMU sample, runtime supplies deterministic downward gravity.

The key contract is unchanged: transient motion may increase disturbance but does not become a new low-frequency gravity direction.

Shared dynamics interprets gravity as transport direction while shake/motion/tap/absolute spin provide a bounded mobility boost and signed spin biases lateral relaxation.

Scene-specific meanings remain bounded:

- Lava + Water: strong disturbance may relocate a tiny number of existing crust cells without creating/destroying their mass;
- Sodium-like + Water and Oil + Fire: motion mainly acts through shared mobility/encounter behavior;
- Moss Garden: projected gravity determines the preferred anti-gravity shoot direction while ordinary water transport moves moisture; strong disturbance may scatter active mites once under event budget.

## `InputFrame` contract

One sanitized `InputFrame` is consumed per logical model tick. It contains normalized gravity, gravity magnitude/confidence, shake/motion/tap/spin, lifecycle intent, common-mode touch, coarse slider semantics and explicit touch-noise input.

Unit-like fields clamp to 0..1, signed direction fields clamp to -1..1, and non-finite input becomes neutral. No hardware timing or unrecorded sensor noise enters deterministic state.

## Capacitive edge input — tested board truth

ES-003/003A physically established deliberately coarse bare-board semantics on GPIO1..GPIO7:

- `cap_a = 0`, `event_a = false`;
- `cap_b = 0`, `event_b = false`;
- `cap_combo` / `event_combo` represent broad common-mode contact;
- a pinch-gated slider exposes active/position/strength;
- isolated local excursions may emit explicit bounded `noise_impulse` / `noise_event`.

Independent A/B touch buttons were not reliable and are not exposed by product scenes.

### Product scene mappings

**Lava + Water**
- slider: inject lava near the selected full-width X at a capped cadence;
- combo: request one bounded lava/water contact pair;
- disturbance/noise may contribute to bounded crust remix.

**Sodium-like + Water**
- slider: inject sodium-like material near the selected full-width X;
- combo: request one bounded sodium-like/water pair;
- subsequent reaction, impulse, fire and steam use shared dynamics.

**Oil + Fire**
- slider: add oil at selected full-width X;
- combo: ignite one existing oil cell;
- propagation/extinction remain shared stateful behavior.

**Moss Garden — ES-009**
- slider: bounded rain pulse around selected X, enabling moisture-driven growth;
- combo: wake/place one inactive mite on moss; if all fixed mite slots are active, seed one moss cell near water instead;
- strong shake/tap/motion/noise: bounded mite scatter;
- tilt: water moves through shared transport and plant-like shoots prefer anti-gravity extension, so orientation changes future ecology through explicit mechanisms.

All product scenes remain autonomous and usable with BOOT + IMU if touch is unavailable.

## External noise is not randomness

`noise_impulse` is explicit external input, not hidden entropy. Live electrical irregularity is replayable only if represented in the ordered `InputFrame` sequence. Scene stochastic choices use model-owned PCG32 only.

## Characterization mode

The separate diagnostic/bring-up target continues to expose raw/baseline/noise/local/common-mode/slider/noise-event information. Product scene code consumes semantic `TouchFrame` fields only.

## Replay evidence

Replay identity is seed/configuration + reset state + tick count + ordered `InputFrame` sequence, not wall-clock timing.

Existing deterministic traces remain. ES-009 adds a 500-tick duplicate Moss Garden trace containing changing gravity, motion and combo input and requires equal model hashes after every tick. Because mite positions/energy/active state and growth energy affect future behavior, those fields are part of Moss Garden hash state.
