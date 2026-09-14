# ESPsand v0 scene catalogue

Scenes are curated demonstrations built on shared material/reaction primitives. They should look distinct even when viewed only as an 8×8 light field.

## Scene 1 — Lava + Water

**Visual identity:** hot orange/red viscous material, dark cooling crust, blue/cyan water, bright steam flashes.

**Autonomous loop:** lava drips or accumulates into a water reservoir, generating steam and crust while the board orientation changes the contact geometry.

**Inputs:**
- cap A: add lava;
- cap B: add water;
- combo/strong disturbance: pressure burst or extra reaction pulse;
- shake/tap: fracture crust and remix contact surfaces;
- tilt: redirects both liquids.

**Must show:** obvious thermal colour gradient, water/lava contact reaction, persistent cooled solid left behind.

## Scene 2 — Sodium-like Reactive Particle + Water

**Visual identity:** bright reactive particle skittering/fizzing over blue water with sharp white/yellow gas/heat flashes.

**Autonomous loop:** one or more bounded reactive particles encounter water and consume themselves while emitting gas/impulse.

**Inputs:**
- cap A: inject one reactive particle;
- cap B: refill/redistribute water;
- shake: increases encounter/agitation;
- tap: optional dramatic but bounded reaction impulse.

**Must show:** local movement caused by reaction impulse, finite reactant lifetime, no unbounded flash loop.

This scene is simulation-only and must not provide real reactive-metal experimental instructions.

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

## Scene 4 — Moss Garden + Mites

**Visual identity:** subdued wet substrate, spreading green moss/plant shoots and 1–3 contrasting moving mite pixels/agents.

**Autonomous loop:** moisture enables growth; biomass spreads; mites wander and selectively nibble biomass, producing small clearings and regrowth cycles.

**Inputs:**
- cap A: rain/water/nutrient pulse;
- cap B: spawn/wake mite or seed burst;
- shake: scatter mites/seeds and temporarily disturb growth;
- tilt: water moves downhill, altering growth zones.

**Must show:** growth over time, at least one agent visibly moving independently, biomass loss caused by feeding, recovery when conditions permit.

## Scene 5 — Tracer / Dissolution Plume

**Visual identity:** localized powder/concentration source blooms into a fluid field whose colour changes dramatically as concentration decreases. The palette may intentionally echo the observed sequence of earth/deep red or orange through lime/green to yellow.

**Autonomous loop:** concentrated source enters water, dissolves/advection-mixes and gradually changes palette with concentration.

**Inputs:**
- cap A: inject concentrated tracer/powder;
- cap B: add clean water/dilution pulse;
- shake/spin: vigorous mixing;
- tilt: moves bulk carrier fluid.

**Must show:** concentration-dependent colour, visible plume transport, eventual dilution rather than random colour cycling.

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

The firmware should expose a stable default scene order. Scene selection may optionally persist across reboot, but no persistence mechanism should complicate early development. A build-time default scene and serial override are useful for debugging.

## Content quality bar

A scene is not complete merely because its named materials exist. It should pass three tests:

1. a viewer can distinguish it from the other scenes without reading serial output;
2. tilt or motion changes what happens in a meaningful way;
3. the scene has an autonomous arc—build-up, interaction, depletion/regrowth/settling—rather than static looping decoration.
