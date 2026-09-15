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

ES-005 adds renderer/output-policy tests for:

- byte-identical output from fixed world/configuration;
- 16×16 -> 8×8 2×2 aggregation;
- preservation of one low-mass hot/fire cell inside a water-majority output block;
- distinct centralized water/lava/moss palette identities;
- deterministic beauty/material-ID/temperature/mass diagnostic modes;
- safe invalid-material fallback/counting;
- hard brightness-ceiling clamping and dense-frame aggregate-load limiting;
- sparse-frame preservation under the same output budget;
- fail-dark zero-load policy;
- randomized full-world render repeatability.

ES-006 adds shared-dynamics tests for:

- exact tracked-mass conservation under whole-cell transport/swaps;
- cardinal and rotated gravity movement;
- water/oil density ordering and steam rise through liquid;
- centralized distinct liquid mobility/viscosity values;
- local pairwise heat exchange, bounded convergence and ambient loss;
- centralized lava-water, sodium-like-water and oil-fire reaction products;
- hard reaction-budget saturation with skipped/dropped candidates rather than recursive runaway;
- finite fire-to-smoke lifetime with mass preserved;
- separation of shake/motion/tap disturbance magnitude from low-frequency gravity direction;
- duplicate full-model dynamics replay under repeated multi-axis normalized input sequences;
- reset restoring the exact initial dynamics state;
- 2,000 randomized dynamics ticks preserving valid material IDs, exact total mass and work-budget bounds.

Later milestones add scene-specific fixed-seed vertical-slice traces and biology/tracer tests when those systems actually exist. Do not add placeholder assertions that imply unimplemented scene behavior is validated.

### 2. Firmware compile

CI compiles both:

- the normal `esp32s3` firmware target;
- the `esp32s3_bringup` minimal target.

Both use the repository's pinned Espressif32/Arduino configuration and compile the shared core library, including renderer/output-limiter and ES-006 dynamics sources. Project warnings are treated seriously; host-native core warnings are errors.

### 3. Formatting/static checks

`tools/format.py --check` applies the repository `.clang-format` contract to C/C++ sources. Keep the quality-tool surface deliberately small and reproducible.

## Deterministic traces

Important model milestones keep compact fixtures containing seed/configuration, deterministic input frames, expected hashes and selected counters.

`test/test_simulation_core/test_main.cpp` locks two foundational ES-004 sequences:

1. PCG32 seed 42, default stream, first six outputs:

```text
2707161783
2068313097
3122475824
2211639955
3215226955
3421331566
```

2. Model seed `0x0123456789ABCDEF`, event budget 2, reaction budget 3:

```text
initial      0x4943A6C732CA020D
after tick 1 0x75A4B3C9249EF546
after tick 2 0xFC14CC3D7A9C7408
after tick 3 0xB411D621F3D3F1C6
after tick 4 0x8F0F30D22E87FB14
```

The final tick also locks event-budget saturation at `used=2`, `dropped=1`.

`Model::state_hash()` is a versioned FNV-1a 64 regression/replay identity over canonical explicit fields, including PRNG state and the row-major world. It is deliberately **not cryptographic** and must not be used as an integrity/authentication mechanism.

ES-006 leaves the original ES-004 hash fixture unchanged. `kDynamicsFixture` adds the dynamics schema discriminator and is exercised as a repeated duplicate-model hash trace under a deterministic gravity/shake/tap/spin sequence. A later dynamics semantic change must update the relevant schema/tests deliberately; unexplained replay drift is a regression.

Renderer output is not part of the model state hash because rendering is a pure projection and cannot affect future model evolution. ES-005 tests byte-identical render frames directly instead.

## Hardware validation gates

Some checks require the actual device. Issues should identify them explicitly and provide a concise procedure plus expected evidence.

Examples:

- actual matrix GPIO/colour order/pixel order;
- IMU address, axis mapping and rate;
- BOOT short/long thresholds;
- touch-capable candidate GPIO behaviour;
- LED current/thermal comfort at configured brightness;
- real frame rate and loop timing;
- 30–60 minute or longer soak without resets.

A remote agent may complete code and CI but must not claim one of these passed without user-provided or machine-collected board evidence.

ES-005 deliberately leaves its 32/255 hard brightness ceiling and 4096-unit aggregate PWM-load envelope `NEEDS_PHYSICAL_VALIDATION`. Automated tests prove limiter arithmetic and gateway integration; they do not prove a sustained electrical/thermal safety rating.

ES-006 also leaves physical simulation timing pending because the current diagnostic runtime does not yet run a product material scene continuously. The exact later benchmark is:

1. run the first vertical-slice scene at its chosen fixed simulation cadence;
2. emit `sim_hz`, rolling/worst `max_tick_us`, render cadence, reaction/event `used/dropped`, and transport/reaction counts in 1 Hz telemetry;
3. exercise all four gravity directions plus repeated shake/tap and active reactions for several minutes;
4. verify no watchdog reset, unbounded backlog or catch-up loop occurs;
5. use measured worst-case headroom, not host timing, when choosing the final fixed tick rate.

## Serial evidence

Diagnostics should make hardware validation easy to paste into an issue/PR. Prefer concise lines such as:

```text
imu ok rate=198Hz g=(+0.03,+0.98) shake=0.02
render fps=60.0 sim=120Hz max_tick_us=...
work event=2/8 drop=0 reaction=3/8 drop=1 moves=27 reacts=3
touch ch=... raw=... base=... z=...
scene=lava_water seed=...
```

Exact schema may differ. Model seed/hash/budget, dynamics work and output-limiter requested/applied/load diagnostics should be added as the runtime begins executing product simulation scenes.

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
