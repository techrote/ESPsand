# ESPsand v0 testing, CI and evidence

## Principle

Automate everything that does not genuinely require the physical board. Never fabricate hardware validation or subjective readability results.

`python tools/ci.py` remains the reproducible verification entry point used by GitHub Actions.

## Automated layers

### Host-native tests

Pure core builds under C++17 with `-Wall -Wextra -Wpedantic -Werror` and Unity tests.

Existing ES-004 through ES-009 suites continue to cover deterministic state/PRNG, renderer/output limiting, shared dynamics/reactions, all four product scenes, bounded ecology/agents, temporal persistence and full-perimeter product worlds.

### Readability regression suite — issue #35

`test_readability` adds four focused guards against the physical blockiness regression:

1. **Coverage preservation:** a physical block sourced by one occupied logical subcell must project with lower RGB energy than the same material filling all four logical subcells.
2. **Uniform-liquid structure:** a completely uniform 16x16 water world must render byte-identically on repeat, expose at least four non-black RGB values, and contain no exact-identical horizontal/vertical run longer than four physical LEDs.
3. **Initial product diversity:** every product scene must begin with at least six non-black physical RGB values and no exact-identical non-black run longer than four LEDs.
4. **Settled product diversity:** after 180 downward-gravity ticks, every product scene must still expose at least four non-black RGB values, have no exact-identical non-black run longer than five LEDs, and retain model invariants.

These tests establish deterministic spatial diversity and protect against obvious large flat blocks. They intentionally do not encode a subjective aesthetic score.

### Existing behavioral regression

The readability work keeps the full existing behavioral suites rather than replacing them with projection tests. In particular:

- Lava + Water still locks deterministic setup/replay, crust+steam reaction, tilt divergence, bounded crust remixing, touch events and long randomized stress;
- Sodium-like + Water still locks finite reactant consumption, shared reaction impulse, fixed-seed replay and bounded inputs;
- Oil + Fire still locks oil-over-water identity, finite fuel consumption, fire expiry/extinction, refill/re-ignition and deterministic replay;
- Moss Garden still locks moisture-gated growth, feeding/energy, starvation, tilt-dependent ecology, independent mite motion, overlay visibility and bounded external events;
- renderer/dynamics/foundation/board/touch suites remain unchanged except where a test explicitly encoded superseded slab quantities.

Two initial-material quantity guards were deliberately recalibrated rather than adding slab material back: Lava now requires at least 12k water and 2k lava mass; Sodium requires over 10k water plus finite reactant. All causal behavior tests remain intact.

### Green implementation probe

CI run **#116 / 34938038488** passed on implementation head `ff2f07057f9e089716fc414fb29a696fc4e1fee2`:

- formatting: pass across **74 C/C++ source files**;
- native tests: **95/95 pass**;
- `test_readability`: **4/4 pass**;
- normal `esp32s3`: **SUCCESS**, RAM **34,824 / 327,680 (10.6%)**, flash **457,749 / 1,310,720 (34.9%)**;
- `esp32s3_bringup`: **SUCCESS**, RAM **31,132 / 327,680 (9.5%)**, flash **408,429 / 1,310,720 (31.2%)**;
- no project/compiler warnings observed; only GitHub Actions Node-runtime deprecation notices.

The final merge gate must repeat the full verification on the exact documented head.

## Deterministic trace policy

The original ES-004 golden trace remains unchanged.

`Model::state_hash()` remains a non-cryptographic replay identity over future-affecting state. Issue #35 deliberately advances Lava/Sodium/Oil schemas to version 3 because their model topology/source geometry changed. Moss remains schema 1 because its correction is renderer-only.

Coverage-aware projection, deterministic structural shading and runtime RGB persistence do not enter model hashes because none can affect future simulation.

Unexplained duplicate-model divergence remains a defect.

## Firmware compile

CI builds:

- normal `esp32s3` product firmware;
- `esp32s3_bringup` hardware-diagnostic firmware.

The readability renderer adds only a small firmware-size increase relative to ES-009; the green implementation probe remained at 10.6% RAM and 34.9% application flash.

## Formatting

`tools/format.py --check` applies the repository clang-format contract to all C/C++ source and test files. The issue-#35 implementation probe checked 74 files successfully.

## Physical validation gates — readability correction #35

CI cannot prove the user's readability complaint is solved. After flashing corrected `main`, evaluate:

1. **Coverage:** thin one-logical-cell streams/shorelines should look visibly less full than dense 2x2 regions instead of inflating into identical square pixels.
2. **Lava:** the hot source should read as a narrow/meandering stream meeting an irregular water body rather than two large rectangles.
3. **Sodium:** individual bright drops should be trackable against an uneven pool before finite reaction events.
4. **Oil:** fuel should read as separated ribbons/pockets and the fire as a spreading/depleting front rather than a broad amber/orange slab.
5. **Moss:** blue/green regions should retain coherent ecology while showing enough internal structure to avoid large featureless blocks.
6. **World-locked texture:** structural contrast should stay attached to material/world structure rather than twinkle like decorative random noise.
7. **Persistence:** frame continuity should remain useful without excessive smear; reaction highlights and mites should remain crisp.
8. **Orientation:** tilt all four directions and confirm shared gravity remains causally understandable.
9. **Perimeter:** all 28 edge LEDs remain available to content; no containment frame may return.
10. **Limiter/timing:** collect normal runtime rates, work counters and requested/applied/load telemetry across scenes.
11. **Soak:** retain the existing 30–60 minute-or-longer multi-scene soak before altering the provisional 32/255 brightness ceiling or 4096-unit load envelope.

Do not infer electrical/thermal safety from software load units or a short visual check.

## Serial evidence

Common telemetry remains scene/seed/tick/hash, measured IMU/simulation/render rates, timing maxima, normalized motion and limiter state. Chemistry/ecology scene lines remain available for reaction and lifecycle evidence.

Renderer blockiness tests do not require new runtime telemetry because their invariants are pure deterministic projection properties.

## PR checklist

Every implementation PR records linked issue, implementation/architecture changes, host-test result, firmware build result, exact CI status, outstanding hardware evidence and reconciled documentation.

## Merge rule

Autonomous work may merge only after required automated checks pass on the exact final head and any non-deferrable acceptance gate is satisfied. Hardware-only/readability observations may remain explicit post-merge validation items when the issue permits remote completion.
