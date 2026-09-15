# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation.

## Required CI layers

The repository's `python tools/ci.py` is the reproducible verification entry point used by GitHub Actions.

### 1. Host-native deterministic tests

The pinned native PlatformIO environment compiles pure core code with C++17 plus `-Wall -Wextra -Wpedantic -Werror` and runs Unity tests.

The ES-004 deterministic substrate suite verifies the fixed world/material registry, PCG32 sequence, replay/reset semantics, work budgets, input sanitization, golden hashes and randomized invariant stress.

ES-005 adds deterministic renderer/output-policy coverage for 2×2 aggregation, important-minority preservation, material/thermal diagnostics, invalid-state fallback, brightness/load limiting and randomized render repeatability.

ES-006 adds shared-dynamics coverage for mass-conserving transport, rotated gravity, density ordering, gas rise, mobility differences, bounded heat exchange, all three shared reactions, reaction-budget saturation, finite fire lifetime, disturbance/gravity separation and randomized deterministic stress.

ES-007 adds 12 Lava + Water / product-input tests covering strong deterministic initialization, autonomous crust/steam evolution, tilt divergence, bounded crust fracture, combo/slider behavior, exact reset, fixed-seed trace replay, renderer identity, motion conditioning and 1,200 randomized scene ticks.

ES-008 adds 13 Sodium-like + Water / Oil + Fire / catalogue tests covering:

- stable three-scene product order and names;
- deterministic Sodium-like + Water initialization with finite reactant over a substantial water field;
- sodium/water contact consuming reactant through the shared reaction, creating gas/fire state and using the common bounded reaction impulse;
- bounded sodium slider injection and combo contact-pair injection;
- a fixed-seed 160-tick Sodium-like + Water duplicate-model trace;
- deterministic Oil + Fire initialization with oil above water and a seeded finite ignition;
- finite fuel consumption, shared fire expiry, smoke production and complete flame extinction during the pre-refill phase;
- bounded oil slider injection and combo ignition;
- a fixed-seed 520-tick Oil + Fire duplicate-model trace spanning refill/re-ignition behavior;
- distinct deterministic initial 8×8 frames for all three current product scenes;
- separate 1,000-tick randomized duplicate-model stress runs for Sodium-like + Water and Oil + Fire, requiring equal hashes, valid materials and bounded event/reaction work throughout.

The first full ES-008 implementation probe passed **79/79 native tests**.

### 2. Firmware compile

CI compiles both:

- the normal `esp32s3` product firmware target;
- the `esp32s3_bringup` minimal hardware-diagnostic target.

The normal target boots the three-scene `SceneRuntime`; the bring-up target remains available for hardware isolation. Both compile the same shared core including renderer, output limiter, dynamics, Lava + Water, energetic scene policy and motion conditioning.

The full ES-008 implementation probe built both targets successfully:

- normal product firmware: RAM **34,592 / 327,680 bytes (10.6%)**, flash **451,973 / 1,310,720 bytes (34.5%)**;
- bring-up firmware: RAM **31,132 / 327,680 bytes (9.5%)**, flash **408,429 / 1,310,720 bytes (31.2%)**.

No project/compiler warnings appeared in that successful run. GitHub Actions emitted only its own Node-runtime deprecation notices.

### 3. Formatting/static checks

`tools/format.py --check` applies the repository `.clang-format` contract to C/C++ sources. The ES-008 implementation probe checked 68 C/C++ source files successfully.

## Deterministic traces

The original ES-004 exact golden trace remains unchanged:

```text
initial      0x4943A6C732CA020D
after tick 1 0x75A4B3C9249EF546
after tick 2 0xFC14CC3D7A9C7408
after tick 3 0xB411D621F3D3F1C6
after tick 4 0x8F0F30D22E87FB14
```

`Model::state_hash()` is a versioned FNV-1a 64 regression/replay identity over canonical explicit fields, including PRNG state and the row-major world. It is deliberately **not cryptographic**.

ES-006 leaves the ES-004 fixture unchanged and adds deterministic dynamics replay. ES-007 adds a Lava + Water scene-schema discriminator plus a 96-tick duplicate-model trace. ES-008 adds Sodium-like + Water and Oil + Fire schema discriminators and duplicate-model traces under explicit scripted inputs. Intentional scene-semantic changes should update the corresponding schema/tests deliberately; unexplained replay divergence is a regression.

Renderer output is not part of the model state hash because rendering is a pure projection and cannot affect future model evolution.

## Hardware validation gates — product catalogue

CI does **not** prove physical panel appearance, handling quality, actual runtime timing or thermal/current safety.

After flashing current `main`, collect board evidence across all three scenes:

1. **Scene order:** long BOOT should cycle Lava + Water -> Sodium-like + Water -> Oil + Fire -> Lava + Water; short BOOT should reset the active scene without changing its seed.
2. **Orientation:** tilt left/right/up/down in each scene and confirm shared gravity follows the intended matrix direction.
3. **Lava + Water identity:** blue/cyan water, hot orange/red lava, dark persistent crust and pale steam; contact should visibly leave cooled solid.
4. **Sodium-like + Water identity:** sparse pale/warm particles over water, finite sharp reaction/fire/steam events and visible local motion from interaction rather than permanent flashing.
5. **Oil + Fire identity:** dim amber oil distinct from water, finite flame fronts, smoke after expiry, and a visibly quiet/extinguished interval before autonomous refill/re-ignition.
6. **Motion response:** shake/tap should increase useful encounter/rearrangement without resets, runaway reactions or a corrupted gravity direction.
7. **Timing:** capture several minutes of 1 Hz telemetry while exercising all scenes. Record `imu_hz`, `sim_hz`, `render_hz`, `max_sim_us`, `max_loop_us`, work used/dropped and scene-specific reaction/lifecycle counters. The ~200 Hz IMU / ~60 Hz simulation / ~60 Hz render values remain provisional until measured on the board.
8. **Output limiter:** confirm every scene reports requested/applied brightness/load and no frame bypasses `MatrixOutput`.
9. **Optional touch:** if usable, confirm slider/combo perform the documented bounded scene action. Independent A/B zones should remain absent.
10. **Soak:** run 30–60 minutes or longer while cycling/handling all scenes. Record resets, USB instability, obvious colour shift or undesirable heating. The 32/255 hard brightness ceiling and 4096-unit load envelope remain `NEEDS_PHYSICAL_VALIDATION`.

Do not convert software load units, visual comfort or short runtime into an electrical/thermal safety claim.

## Serial evidence — ES-008

Common runtime lines identify the active scene, seed/tick/hash, measured rates, timing maxima and normalized motion. Scene lines expose material and bounded-work evidence, for example:

```text
runtime scene=sodium_water seed=... tick=... hash=... imu_hz=... sim_hz=... render_hz=... max_sim_us=...
scene.sodium_water water=... sodium=... fire=... steam=... react=... impulse=... move=... event=used/limit/drop reaction=used/limit/drop led=requested/applied load=... limited=...

runtime scene=oil_fire ...
scene.oil_fire water=... oil=... fire=... smoke=... react=... expired=... move=... inject_oil=... auto_ignite=... combo_ignite=...
```

Telemetry is evidence plumbing, not physical evidence by itself.

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
