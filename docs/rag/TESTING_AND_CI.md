# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation.

## Required CI layers

The repository's `python tools/ci.py` is the reproducible verification entry point used by GitHub Actions.

### 1. Host-native deterministic tests

The pinned native PlatformIO environment compiles pure core code with C++17 plus `-Wall -Wextra -Wpedantic -Werror` and runs Unity tests.

The ES-004 deterministic substrate suite verifies:

- the fixed 16×16/256-cell world, row-major access and boundary rejection;
- centralized stable material IDs and material/mass accounting;
- exact PCG32 fixture output;
- same-seed/same-input replay identity;
- different-seed controlled stochastic divergence;
- reset and reseed semantics;
- event/reaction work-budget saturation;
- external touch/noise input separation from PRNG state;
- input sanitization for out-of-range, NaN and infinite values;
- a versioned golden multi-tick state trace;
- 5,000 randomized ticks on duplicate models while checking invariants, accounting, bounded work and randomized out-of-bounds probes.

ES-005 adds renderer/output-policy tests for deterministic 2×2 aggregation, important-minority preservation, palette identity, diagnostic modes, invalid-material fallback, brightness/load limiting and randomized render repeatability.

ES-006 adds shared-dynamics tests for exact mass conservation, rotated gravity, density ordering, gas rise, mobility differences, thermal convergence, the three shared reaction families, reaction-budget saturation, finite fire lifetime, disturbance/gravity separation, deterministic full-model replay and 2,000 randomized dynamics ticks.

ES-007 adds 12 Lava + Water / product-input tests covering:

- deterministic strong scene initialization with substantial water and lava masses;
- autonomous contact producing persistent crust and steam while pre-injection total mass remains conserved;
- materially different flow geometry under different gravity directions;
- shake-driven bounded crust fracture without mass loss;
- common-mode combo using the ordinary shared lava-water reaction path;
- pinch-slider lava injection at a bounded horizontal position;
- exact fixed-seed reset;
- a 96-tick fixed-seed duplicate-model state-hash trace containing gravity changes, shake, tap, combo and slider input;
- renderer distinction between water, hot lava, cooled crust and steam;
- stable low-pass gravity without false shake under static input;
- separation of a motion impulse/tap/spin from low-pass gravity;
- 1,200 randomized scene ticks with duplicate replay, valid material state and bounded work.

The first full ES-007 integration probe ran **66/66 native tests successfully**. Final merge still requires the same exact-head CI gate after documentation/cleanup changes.

### 2. Firmware compile

CI compiles both:

- the normal `esp32s3` product firmware target;
- the `esp32s3_bringup` minimal hardware-diagnostic target.

ES-007 changes the normal target to boot `SceneRuntime` and Lava + Water. The bring-up target remains available for hardware isolation. Both compile the same shared core, including renderer, output limiter, ES-006 dynamics, ES-007 scene code and the normalized motion interpreter.

The first full ES-007 integration probe built both targets successfully. Normal product firmware used approximately 10.6% reported RAM and 34.2% of the configured application flash partition in that CI run; these are compile-time size reports, not runtime performance evidence.

### 3. Formatting/static checks

`tools/format.py --check` applies the repository `.clang-format` contract to C/C++ sources. Keep the quality-tool surface deliberately small and reproducible.

## Deterministic traces

Important model milestones keep compact fixtures containing seed/configuration, deterministic input frames, state-hash comparisons and selected counters.

The original ES-004 exact golden trace remains unchanged:

```text
initial      0x4943A6C732CA020D
after tick 1 0x75A4B3C9249EF546
after tick 2 0xFC14CC3D7A9C7408
after tick 3 0xB411D621F3D3F1C6
after tick 4 0x8F0F30D22E87FB14
```

`Model::state_hash()` is a versioned FNV-1a 64 regression/replay identity over canonical explicit fields, including PRNG state and the row-major world. It is deliberately **not cryptographic**.

ES-006 leaves the original ES-004 hash fixture unchanged and adds deterministic dynamics replay. ES-007 additionally gives `kLavaWater` its own scene-schema discriminator and runs two independently constructed models through the same 96-tick scripted input trace, requiring equal hashes before and after every tick. This catches any nondeterministic divergence while allowing intentional scene evolution to be versioned deliberately rather than silently inheriting an unrelated old golden value.

Renderer output is not part of the model state hash because rendering is a pure projection and cannot affect future model evolution.

## Hardware validation gates — ES-007

ES-007 is the first milestone where the normal firmware is a continuously running product scene, so physical validation is now meaningful. CI **does not** prove any item below.

After flashing current `main`, collect this board evidence:

1. **Orientation:** tilt the physical board left/right/up/down and confirm liquid movement follows the intended matrix direction. Record any axis/sign mismatch; do not compensate in scene code if the board transform is wrong.
2. **Visual identity:** verify blue/cyan water, bright orange/red hot lava, visibly darker cooled crust, and pale/bright steam or reaction highlights. Confirm matrix pixel order and colour order still look correct.
3. **Contact behavior:** allow the autonomous scene to run and confirm visible lava-water contact produces steam and persistent crust rather than merely colour-changing in place.
4. **Motion response:** shake/tap the board and confirm a useful but bounded disturbance/fracture response without scene resets, runaway reactions or sticky motion.
5. **Timing:** capture at least several minutes of 1 Hz serial telemetry while exercising all four gravity directions and repeated reactions. Record `imu_hz`, `sim_hz`, `render_hz`, `max_sim_us`, `max_loop_us`, event/reaction used/dropped counts and transport/reaction counts. The provisional targets are about 200 Hz IMU polling and 60 Hz simulation/rendering; measured data decides whether those rates remain.
6. **Output limiter:** confirm telemetry continues to report requested/applied brightness and estimated load; no product frame may bypass `MatrixOutput`.
7. **Soak:** run 30–60 minutes or longer with active reactions. Record any reset, USB instability, visible colour shift or undesirable heating. The existing 32/255 hard brightness ceiling and 4096-unit aggregate-load envelope remain `NEEDS_PHYSICAL_VALIDATION`; ES-007 currently requests 28/255 and may be reduced automatically by the limiter.
8. **Optional touch:** if bare-board touch is usable, confirm slider position approximately selects lava-injection X and combo causes a bounded reaction pulse. Touch failure/noise must not prevent the autonomous BOOT+IMU scene from working.

Do not convert visual comfort, short runtime or software load units into a claimed electrical current/thermal rating.

## Serial evidence

The ES-007 product runtime emits compact 1 Hz lines containing, as applicable:

```text
runtime scene=lava_water seed=... tick=... hash=... imu_hz=... sim_hz=... render_hz=... max_sim_us=... max_loop_us=... g=(...,... ) shake=... tap=...
scene.lava_water water=... lava=... crust=... steam=... react=... move=... fracture=... inject_lava=... inject_water=... touch_lava=... burst=... event=used/limit/drop reaction=used/limit/drop render_minor=... led=requested/applied load=... limited=...
```

This telemetry is evidence plumbing, not evidence by itself. Physical claims require captured board output.

## PR checklist

Every implementation PR should state:

- linked issue;
- implementation summary;
- architectural decisions/deviations;
- host tests run and result;
- firmware build result;
- automated CI result;
- required hardware validation still outstanding;
- docs updated for any resolved assumption or model contract.

## Merge rule

An autonomous issue explicitly authorizes the implementing agent to merge its own PR **only after the required automated checks pass** and any issue-specific non-deferrable acceptance gates are satisfied.

If CI infrastructure itself is broken for reasons unrelated to the PR, fix or explicitly reconcile it; do not simply merge around it.
