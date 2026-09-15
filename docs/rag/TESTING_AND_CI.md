# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation or subjective readability results.

`python tools/ci.py` remains the reproducible verification entry point used by GitHub Actions.

## Automated layers

### Host-native tests

Pure core builds under C++17 with `-Wall -Wextra -Wpedantic -Werror` and Unity tests.

Existing ES-004 through ES-008 suites continue to cover deterministic state/PRNG, renderer/output limiting, shared dynamics/reactions, Lava + Water, Sodium-like + Water and Oil + Fire.

ES-009 adds **12 Moss Garden / presentation tests**:

1. every product scene initializes with zero wall mass reserved for containment;
2. deterministic Moss Garden initialization contains water, moss and two active mites;
3. removing all water prevents growth/reinforcement;
4. a wet habitat produces bounded growth/reinforcement;
5. mites feed on moss and gain energy;
6. mites starve/deactivate without biomass while invariants remain valid;
7. a 500-tick duplicate-model trace locks deterministic ecology/agent paths;
8. opposite tilt directions change future moss geometry/state;
9. mite coordinates move independently of material cells;
10. mite overlay remains visible after 16x16 -> 8x8 projection;
11. temporal RGB persistence is deterministic and bounded;
12. slider rain and shake scatter remain inside event budget.

The existing energetic catalogue test is also updated for the four-scene order and Moss Garden name.

The first full ES-009 implementation probe passed **91/91 native test cases**. Existing Lava, Sodium, Oil, renderer and shared-dynamics suites remained green after the product-scene re-composition.

### Firmware compile

CI builds:

- normal `esp32s3` product firmware;
- `esp32s3_bringup` hardware-diagnostic firmware.

Successful ES-009 probe figures:

- product: **34,824 / 327,680 RAM (10.6%)**, **456,241 / 1,310,720 flash (34.8%)**;
- bring-up: **31,132 / 327,680 RAM (9.5%)**, **408,429 / 1,310,720 flash (31.2%)**.

The product build compiled Moss Garden, fixed agent state and scene-effects persistence/overlay code. No project/compiler warnings were observed; GitHub Actions emitted only its own Node-runtime deprecation notices.

### Formatting

The successful probe checked **73 C/C++ source files** under the repository clang-format contract.

## Deterministic trace policy

The original ES-004 golden trace remains unchanged.

`Model::state_hash()` remains a non-cryptographic versioned replay identity over explicit future-affecting state. ES-009 deliberately bumps schemas for the three re-composed chemistry scenes and gives Moss Garden its own schema.

Moss Garden hashing includes mite positions/energy/active state and growth energy. RGB presentation history is excluded because it cannot affect future simulation.

Intentional scene semantic changes require deliberate schema/test updates. Unexplained duplicate-model divergence is a defect.

## Physical validation gates — ES-009

CI does **not** prove the user's core readability complaint is solved. After flashing current ES-009 `main`, collect physical evidence:

1. **Perimeter recovery:** confirm the outer physical ring is no longer a static/dim containment frame. All 28 perimeter LEDs should be available to ordinary scene content as material reaches their 2x2 source blocks.
2. **Scene order:** long BOOT cycles Lava -> Sodium -> Oil -> Moss -> Lava; short BOOT resets current scene/seed.
3. **Orientation:** tilt all four directions and verify shared gravity follows the intended matrix direction.
4. **Lava readability:** look for a coherent central hot source entering a broad blue basin, with dark crust and pale steam after contact.
5. **Sodium readability:** paired bright drops should be visible before finite reaction/fire/steam events.
6. **Oil readability:** broad oil layer and left-originating flame front should read as fuel/front behavior; the fire must still visibly gutter out before refill/re-ignition.
7. **Moss readability:** green biomass/plant growth should remain stable enough to recognize, while 1–3 magenta/white mites move independently and feeding eventually creates/rearranges clearings.
8. **Tilt-to-ecology causality:** leave Moss Garden tilted in different orientations and confirm water/moisture placement alters where later growth succeeds; this is intentionally slower than the chemistry scenes.
9. **Persistence quality:** confirm motion is easier to track and less jittery without feeling excessively smeared/laggy. Reaction highlights and mites should remain crisp because they are applied after base-frame persistence.
10. **Motion/touch:** shake should scatter mites/usefully disturb scenes without corrupting gravity. If touch is usable, slider rain/material injection and combo scene events should be bounded; A/B zones remain absent.
11. **Timing:** collect `imu_hz`, `sim_hz`, `render_hz`, `max_sim_us`, `max_loop_us`, work counters and scene counters for several minutes across all scenes.
12. **Limiter/soak:** confirm requested/applied brightness/load telemetry remains present and run the existing 30–60 minute-or-longer multi-scene soak before changing the 32/255 / 4096-unit provisional LED policy.

Do not infer electrical/thermal safety from software load units or a short visual check.

## Serial evidence

Common runtime telemetry remains scene/seed/tick/hash, measured IMU/simulation/render rates, timing maxima, normalized motion and limiter state.

ES-009 adds a Moss line such as:

```text
scene.moss_garden water=... moss=... mites=... growth_energy=... grow=... reinforce=... mite_move=... feed=... starved=... rain=... seed=... scatter=... event=used/limit/drop led=requested/applied load=... limited=...
```

Chemistry scene lines remain available for reaction/material lifecycle evidence.

Telemetry is evidence plumbing, not physical evidence by itself.

## PR checklist

Every implementation PR records linked issue, implementation/architecture changes, host-test result, firmware build result, exact CI status, outstanding hardware evidence and reconciled documentation.

## Merge rule

Autonomous issue work may merge only after required automated checks pass on the exact final head and any non-deferrable acceptance gate is satisfied. Hardware-only/readability observations may remain explicit post-merge validation items when the issue permits remote completion.
