# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation.

## Required CI layers

The repository's `python tools/ci.py` is the reproducible verification entry point used by GitHub Actions.

### 1. Host-native deterministic tests

The pinned native PlatformIO environment compiles pure core code with C++17 plus `-Wall -Wextra -Wpedantic -Werror` and runs Unity tests.

The current suite covers foundation/board/input logic plus the ES-004 deterministic substrate. ES-004 specifically verifies:

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

Later milestones add transport conservation, density/buoyancy, heat, reactions, biology and renderer tests when those systems actually exist. Do not add placeholder assertions that imply unimplemented physics is validated.

### 2. Firmware compile

CI compiles both:

- the normal `esp32s3` firmware target;
- the `esp32s3_bringup` minimal target.

Both use the repository's pinned Espressif32/Arduino configuration and compile the shared core library. Project warnings are treated seriously; host-native core warnings are errors.

### 3. Formatting/static checks

`tools/format.py --check` applies the repository `.clang-format` contract to C/C++ sources. Keep the quality-tool surface deliberately small and reproducible.

## Deterministic traces — ES-004 contract

Important model milestones keep compact fixtures containing seed/configuration, deterministic input frames, expected hashes and selected counters.

`test/test_simulation_core/test_main.cpp` locks two foundational sequences:

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

An intentional model-semantic change may alter these fixtures. Update the fixture and explanatory documentation in the same PR; an unexplained hash drift is a regression.

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

A remote agent may complete code and CI but must not claim one of these passed without user-provided or machine-collected board evidence. ES-004 itself changes only pure model contracts and does not introduce a new physical-board acceptance gate.

## Serial evidence

Diagnostics should make hardware validation easy to paste into an issue/PR. Prefer concise lines such as:

```text
imu ok rate=198Hz g=(+0.03,+0.98) shake=0.02
render fps=60.0 sim=120Hz max_tick_us=...
touch ch=... raw=... base=... z=...
scene=lava_water seed=...
```

Exact schema may differ. Model seed/hash/budget diagnostics should be added when the runtime begins executing product simulation scenes.

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
