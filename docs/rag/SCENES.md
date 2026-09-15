# ESPsand v0 scene catalogue

Scenes are curated demonstrations built on shared material/reaction primitives. They should look distinct even when viewed only as an 8×8 light field.

## Scene 1 — Lava + Water — ES-007 baseline

**Visual identity:** hot orange/red viscous lava, deliberately dark cooled crust, blue/cyan water and bright steam/reaction highlights.

**Autonomous loop:** the deterministic initial state contains a substantial water reservoir, a hot lava body and one seeded contact point. The scene then periodically injects bounded lava and slower water replenishment from the upper interior so it remains active without user input. All movement, heat exchange, lava/water -> crust/steam conversion and gas rise use the shared ES-006 mechanics.

**Implemented inputs:**

- tilt redirects shared gravity transport for both liquids and the buoyant steam phase;
- shake/tap/general motion can relocate a small bounded number of existing crust cells, reopening contact surfaces without creating or destroying their mass;
- the accepted pinch-gated coarse slider injects one bounded lava cell near its 0..1 horizontal position at a capped cadence;
- the accepted common-mode combo event injects one adjacent lava/water pair into available interior space, after which the ordinary shared reaction engine performs the conversion;
- isolated touch-noise input contributes only through the same bounded disturbance interpretation; it is not hidden randomness.

The physically rejected independent `cap_a` / `cap_b` semantics are **not** resurrected for this scene. Lava + Water remains fully usable with BOOT + IMU when capacitive sensing is unavailable.

**Lifecycle:** short BOOT restores the exact configured seed/initial state. While this is the only product scene, long BOOT follows the product “next scene” intent by wrapping to Lava + Water with the next deterministic seed. ES-008 will replace that one-scene wrap with real scene advance when Scene 2 exists.

**Must show / automated evidence:** fixed-seed host tests require deterministic initialization/replay, meaningful multi-axis tilt divergence, lava/water contact producing persistent crust plus steam, bounded shake fracture, bounded touch injection, exact reset, renderer distinction and long randomized bounded replay. Physical visual quality and handling response remain a board-validation gate; CI cannot claim those observations.

## Scene 2 — Sodium-like Reactive Particle + Water

**Visual identity:** bright reactive particle skittering/fizzing over blue water with sharp white/yellow gas/heat flashes.

**Autonomous loop:** one or more bounded reactive particles encounter water and consume themselves while emitting gas/impulse.

**Inputs:**
- cap A: inject one reactive particle;
- cap B: refill/redistribute water;
- shake: increases encounter/agitation;
- tap: optional dramatic but bounded reaction impulse.

**Must show:** local movement caused by reaction impulse, finite reactant lifetime, no unbounded flash loop.

This scene is simulation-only and must not provide real reactive-metal experimental instructions. Its capacitive mappings must be reconciled against the accepted ES-003A slider/combo truth before implementation; the aspirational A/B wording above is not authorization to re-enable rejected bare-board zones.

## Scene 3 — Oil Fire

**Visual identity:** dark/amber low-density oil floating above water or empty space; orange/yellow flame front; dim smoke/embers.

**Autonomous loop:** oil pools and spreads; ignition consumes it; flames propagate where fuel/heat permit and eventually gutter out.

**Inputs:**
- cap A: add oil/fuel;
- cap B: ignition/heat pulse;
- combo: flash event constrained by available fuel;
- tilt: redistributes liquid fuel;
- shake: splashes/rearranges fuel and embers.

**Must show:** fuel depletion, flame spread, visible extinction rather than permanent decorative fire.

As with Scene 2, future implementation must translate the old A/B aspiration into the actually supported ES-003A slider/combo semantics rather than exposing nonexistent independent zones.

## Scene 4 — Moss Garden + Mites

**Visual identity:** subdued wet substrate, spreading green moss/plant shoots and 1–3 contrasting moving mite pixels/agents.

**Autonomous loop:** moisture enables growth; biomass spreads; mites wander and selectively nibble biomass, producing small clearings and regrowth cycles.

**Inputs:**
- cap A: rain/water/nutrient pulse;
- cap B: spawn/wake mite or seed burst;
- shake: scatter mites/seeds and temporarily disturb growth;
- tilt: water moves downhill, altering growth zones.

**Must show:** growth over time, at least one agent visibly moving independently, biomass loss caused by feeding, recovery when conditions permit.

Future capacitive mappings remain subject to the accepted coarse slider/combo hardware truth.

## Scene 5 — Tracer / Dissolution Plume

**Visual identity:** localized powder/concentration source blooms into a fluid field whose colour changes dramatically as concentration decreases. The palette may intentionally echo the observed sequence of earth/deep red or orange through lime/green to yellow.

**Autonomous loop:** concentrated source enters water, dissolves/advection-mixes and gradually changes palette with concentration.

**Inputs:**
- cap A: inject concentrated tracer/powder;
- cap B: add clean water/dilution pulse;
- shake/spin: vigorous mixing;
- tilt: moves bulk carrier fluid.

**Must show:** concentration-dependent colour, visible plume transport, eventual dilution rather than random colour cycling.

Future capacitive mappings remain subject to the accepted coarse slider/combo hardware truth.

## Scene 6+ candidates

Select at least one after hero-scene profiling:

- ice/melt/refreeze;
- volcano/pressure vent;
- smoke/thermal convection;
- acid-like corrosion fantasy material;
- seed/fire/regrowth ecology;
- "everything box" using a carefully bounded subset of materials.

Selection criterion is **maximum new visible behaviour per implementation complexity**, not thematic completeness.

## Scene order and persistence

The firmware should expose a stable default scene order. ES-007 is the first product scene and therefore boots directly by default. Until ES-008 adds Scene 2, long BOOT wraps to the same scene with a deterministic seed increment; short BOOT restores the current seed exactly. Scene selection may optionally persist across reboot later, but no persistence mechanism should complicate early development.

## Content quality bar

A scene is not complete merely because its named materials exist. It should pass three tests:

1. a viewer can distinguish it from the other scenes without reading serial output;
2. tilt or motion changes what happens in a meaningful way;
3. the scene has an autonomous arc—build-up, interaction, depletion/regrowth/settling—rather than static looping decoration.
