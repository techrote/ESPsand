# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation.

## Required CI layers

The repository's `python tools/ci.py` is the reproducible verification entry point used by GitHub Actions.

### 1. Host-native deterministic tests

The pinned native PlatformIO environment compiles pure core code with C++17 plus `-Wall -Wextra -Wpedantic -Werror` and runs Unity tests.

The ES-004 suite locks the deterministic world/material/PRNG/InputFrame substrate, including exact PRNG output, state-hash trace, reset/reseed behaviour, bounds/accounting, work budgets and randomized replay stress.

ES-005 adds renderer/output-policy coverage for:

- byte-identical output for a fixed world and render configuration;
- 16×16 -> 8×8 2×2 aggregation;
- preservation of a small high-priority fire cell within a water-majority block;
- distinct centralized palette identities;
- deterministic beauty/material/temperature/mass diagnostic modes;
- safe handling/counting of an invalid material ID;
- hard brightness-ceiling clamping;
- aggregate dense-frame load limiting;
- sparse-frame preservation under the load budget;
- fail-dark zero-load policy;
- randomized full-world render repeatability.

Later milestones add transport conservation, density/buoyancy, heat, reactions and biology only when those systems actually exist.

### 2. Firmware compile

CI compiles both the normal `esp32s3` firmware and the `esp32s3_bringup` minimal target with the pinned toolchain. Both link the shared core library, including the ES-005 renderer/limiter sources.

### 3. Formatting/static checks

`tools/format.py --check` applies the repository `.clang-format` contract to C/C++ sources. Keep the quality-tool surface deliberately small and reproducible.

## Deterministic traces

`Model::state_hash()` remains the ES-004 versioned FNV-1a replay identity over canonical explicit state. Renderer output is not inserted into the model hash because rendering is a pure projection and does not affect future simulation state.

ES-005 instead tests byte-identical render frames directly. An intentional palette/aggregation/tone-map change should update renderer expectations in the same PR and explain the visual-semantic change.

## Hardware validation gates

Some checks require the actual device. Examples include matrix electrical/thermal behaviour, real frame rate, current draw, and extended soak stability.

ES-005 automated tests prove limiter arithmetic and that `MatrixOutput` owns the limiter path. They do **not** prove that 32/255 or the 4096 software load envelope is a safe sustained electrical setting. Those values remain provisional pending physical current/thermal evidence.

For sustained-output validation, prefer a 30–60 minute or longer soak with representative and deliberately dense patterns. Record the applied limiter state and measured evidence rather than inferring safety from a short visual check.

## Serial evidence

Diagnostics should remain compact and machine-readable. As runtime integration grows, useful fields include scene/mode, frame/simulation rates, IMU status, seed/hash/work counters, and output-limiter requested/applied brightness plus load state.

## PR checklist

Every implementation PR should state:

- linked issue;
- implementation summary;
- architectural decisions/deviations;
- host tests run and result;
- firmware build result;
- automated CI result;
- required hardware validation still outstanding;
- docs updated for any resolved assumption/model/render contract.

## Merge rule

An autonomous issue explicitly authorizes the implementing agent to merge its own PR only after required automated checks pass and any issue-specific non-deferrable acceptance gates are satisfied.

If CI infrastructure itself is broken for reasons unrelated to the PR, fix or explicitly reconcile it; do not merge around it.
