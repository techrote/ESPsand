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

**Lifecycle:** short BOOT restores the exact configured seed/initial state. Long BOOT advances to Sodium-like + Water under the ES-008 three-scene product catalogue.

**Must show / automated evidence:** fixed-seed host tests require deterministic initialization/replay, meaningful multi-axis tilt divergence, lava/water contact producing persistent crust plus steam, bounded shake fracture, bounded touch injection, exact reset, renderer distinction and long randomized bounded replay. Physical visual quality and handling response remain a board-validation gate; CI cannot claim those observations.

## Scene 2 — Sodium-like Reactive Particle + Water — ES-008 baseline

**Visual identity:** small pale/warm reactive particles moving over a blue water field, with sharp fire/steam/heat events and buoyant gas after contact.

**Autonomous loop:** a deterministic water reservoir contains a small finite set of sodium-like particles, including one seeded near-contact placement. Particles use the shared gravity/density transport. Contact with water uses the centralized ES-006 sodium-like + water reaction, creating finite fire plus steam and the common bounded reaction impulse. Sparse autonomous reactant injection and slower water refill keep the scene demonstrable without touch.

**Implemented inputs:**

- tilt redirects the ordinary shared particle/liquid transport;
- shake/tap/motion increase the shared bounded mobility/disturbance term and therefore encounter rate without creating scene-local reaction loops;
- the coarse slider injects one sodium-like cell near the selected horizontal position at a capped cadence;
- combo inserts one adjacent sodium-like/water contact pair into available interior space, after which the shared reaction engine performs the reaction and impulse;
- the scene remains complete with BOOT + IMU only.

Independent A/B touch zones remain disabled because the tested bare board did not support reliable two-zone operation.

**Must show / automated evidence:** finite reactant consumption, bounded local reactions/impulses, gas/fire products, fixed-seed replay and bounded work. The particle is not animated along a scripted “skitter” path: visible reaction motion comes from the shared transport/motion fields and common reaction impulse.

This scene is simulation-only. It contains no real reactive-metal experimental instructions, quantities or handling guidance.

## Scene 3 — Oil + Fire — ES-008 baseline

**Visual identity:** dim amber/brown low-density oil layered above blue water, bright orange/yellow finite flame fronts and dim smoke after burn-out.

**Autonomous loop:** a deterministic water layer supports an oil pool with one seeded ignition. Oil uses the shared lower density and mobility metadata, so it behaves differently from water. The centralized oil + fire reaction converts adjacent fuel into finite fire; generic ES-006 fire lifetime then produces smoke. Each long autonomous cycle contains a quiet burn/extinction interval followed by sparse fuel refill and one re-ignition so the scene has visible depletion and recovery rather than permanent decorative fire.

**Implemented inputs:**

- tilt redistributes oil through shared density/mobility transport;
- shake/motion raise the ordinary bounded mobility term and rearrange fuel/fire/smoke without scene-local fluid rules;
- the coarse slider adds one oil cell near the selected horizontal position at a capped cadence;
- combo ignites one existing oil cell through a bounded event, after which propagation uses the common oil/fire reaction;
- the scene remains fully usable with BOOT + IMU alone.

Independent A/B touch zones remain disabled.

**Must show / automated evidence:** oil begins above water, fuel is consumed only through stateful ignition/reaction, fire has finite lifetime, smoke remains after expiry, and a pre-refill interval reaches visible extinction. Fixed-seed and randomized duplicate-model traces lock determinism and bounded work.

This is stylized simulation content only; it is not fuel or ignition guidance.

## Scene 4 — Moss Garden + Mites

**Visual identity:** subdued wet substrate, spreading green moss/plant shoots and 1–3 contrasting moving mite pixels/agents.

**Autonomous loop:** moisture enables growth; biomass spreads; mites wander and selectively nibble biomass, producing small clearings and regrowth cycles.

**Inputs:**
- cap A: rain/water/nutrient pulse;
- cap B: spawn/wake mite or seed burst;
- shake: scatter mites/seeds and temporarily disturb growth;
- tilt: water moves downhill, altering growth zones.

**Must show:** growth over time, at least one agent visibly moving independently, biomass loss caused by feeding, recovery when conditions permit.

Future capacitive mappings remain subject to the accepted coarse slider/combo hardware truth; the old A/B wording above is aspirational scene intent, not authorization to expose unsupported bare-board zones.

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

## Scene order and persistence — ES-008 baseline

The stable product order is now:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Lava + Water
```

Cold boot starts at Lava + Water. Short BOOT resets the current scene at exactly its current seed. Long BOOT advances one entry and increments the deterministic seed before initializing the next scene. This makes scene changes visibly meaningful now that three product scenes exist.

Scene selection is not persisted across reboot in v0 yet; avoiding persistence complexity remains intentional.

## Content quality bar

A scene is not complete merely because its named materials exist. It should pass three tests:

1. a viewer can distinguish it from the other scenes without reading serial output;
2. tilt or motion changes what happens in a meaningful way;
3. the scene has an autonomous arc—build-up, interaction, depletion/regrowth/settling—rather than static looping decoration.
