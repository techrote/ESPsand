# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation.

## Required CI layers

### 1. Host-native deterministic tests

Compile the pure core on the CI host and test:

- world initialization;
- fixed-seed determinism;
- button/input state machines where hardware-independent;
- gravity-coordinate transforms;
- transport conservation fixtures;
- density/buoyancy tendencies;
- heat diffusion bounds;
- reaction products and budgets;
- biology/agent boundedness;
- renderer aggregation and brightness limiter;
- randomized stress/fuzz-like tick sequences for bounds/invariant failures.

### 2. Firmware compile

CI must compile the target ESP32-S3 firmware with the repository's pinned toolchain/dependencies. Once the exact board profile is known, CI should use it or a documented equivalent.

Warnings introduced by project code should be treated seriously; foundation work may choose the exact policy.

### 3. Formatting/static checks

Use deterministic formatter/linter checks appropriate to the selected C++/PlatformIO stack. Avoid a huge quality-tool suite whose maintenance cost exceeds this project's size.

## Deterministic traces

For important model milestones, keep small fixtures such as:

```text
seed
initial world
N input frames
expected final state hash
selected intermediate counters
```

These catch accidental behavioural drift while remaining cheaper than image/video golden tests.

When intentionally changing model semantics, update fixtures in the same PR with an explanation.

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

## Serial evidence

Diagnostics should make hardware validation easy to paste into an issue/PR. Prefer concise lines such as:

```text
imu ok rate=198Hz g=(+0.03,+0.98) shake=0.02
render fps=60.0 sim=120Hz max_tick_us=...
touch ch=... raw=... base=... z=...
scene=lava_water seed=...
```

Exact schema may differ.

## PR checklist

Every implementation PR should state:

- linked issue;
- implementation summary;
- architectural decisions/deviations;
- host tests run and result;
- firmware build result;
- automated CI result;
- required hardware validation still outstanding;
- docs updated for any resolved hardware assumption.

## Merge rule

An autonomous issue explicitly authorizes the implementing agent to merge its own PR **only after the required automated checks pass** and any issue-specific non-deferrable acceptance gates are satisfied.

If CI infrastructure itself is broken for reasons unrelated to the PR, fix or explicitly reconcile it; do not simply merge around it.
