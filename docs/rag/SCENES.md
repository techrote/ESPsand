# ESPsand v0 scene catalogue

Scenes are curated demonstrations built on shared material/reaction primitives. On an 8x8 matrix the quality bar is macroscopic readability: a viewer should be able to recognize broad bodies, fronts and agents rather than infer meaning from isolated flickering cells.

## Product boundary policy — ES-009

Product scenes no longer reserve a one-cell logical wall around the 16x16 world. `DynamicsEngine` already rejects out-of-bounds movement, so the explicit wall ring was redundant containment and, after 2x2 downsampling, visually contaminated every physical perimeter pixel.

All 16x16 logical positions are now available to product content, including the four logical edges. Consequently all **28 physical perimeter LEDs** of the 8x8 matrix may participate in animation. Explicit wall material remains valid for test fixtures or future deliberately authored obstacles; it is no longer the default product-scene boundary.

ES-009 also re-composes the first three scenes around larger semantic shapes and adds bounded render-only temporal persistence. Persistence does not alter model state or PRNG evolution. Reaction highlights and agent overlays are applied after persistence so important events remain crisp.

## Scene 1 — Lava + Water — ES-009 composition

**Visual identity:** a central orange/red lava source descending into a coherent full-width blue/cyan basin, producing dark crust and pale/bright steam.

**Autonomous loop:** water occupies the lower full-width band; a four-logical-cell-wide lava source begins at the top centre, with an early central contact path to make the reaction legible quickly. Periodic lava arrives through a narrow central vent while slower water replenishment targets the basin. Gravity, heat, gas rise and lava/water -> crust/steam remain shared ES-006 mechanics.

**Inputs:** tilt redirects shared transport; strong motion may perform bounded mass-preserving crust remixing; slider adds lava at the selected full-width X; combo inserts one bounded lava/water contact pair. Independent A/B touch zones remain disabled.

The layout change is deliberate and increments the Lava + Water scene schema to version 2.

## Scene 2 — Sodium-like + Water — ES-009 composition

**Visual identity:** paired pale/warm reactant drops falling toward a coherent blue pool, followed by sharp finite fire/steam events.

**Autonomous loop:** water forms a full-width lower pool. Sodium-like material begins as a few paired logical cells so each reactant reads as a larger physical feature rather than a single downsampled speck. Autonomous replenishment inserts paired top drops; slower water refill maintains the pool. Contact still uses the centralized sodium-like + water reaction and shared reaction impulse.

**Inputs:** tilt controls ordinary transport; shake/tap/motion increase shared mobility; slider injects sodium-like material at the selected full-width X; combo inserts one bounded reactant/water pair. The scene remains complete with BOOT + IMU only.

The composition change increments the Sodium-like + Water scene schema to version 2. This remains simulation-only content and contains no real reactive-metal procedure.

## Scene 3 — Oil + Fire — ES-009 composition

**Visual identity:** a broad amber low-density oil layer above a thin blue water layer, with a directional orange/yellow flame front and dim smoke after burn-out.

**Autonomous loop:** oil spans the width above water. Initial ignition starts at the left edge so propagation can read as a front rather than scattered flashing. Shared oil/fire reactions consume finite fuel; shared fire lifetime produces smoke and allows a visible quiet/extinguished interval before sparse refill and re-ignition.

**Inputs:** tilt redistributes the shared oil layer; shake/motion rearrange fuel/fire/smoke through shared transport disturbance; slider adds oil at the selected full-width X; combo ignites one existing oil cell.

The composition change increments the Oil + Fire scene schema to version 2. This is stylized simulation content, not fuel or ignition guidance.

## Scene 4 — Moss Garden + Mites — ES-009

**Visual identity:** a subdued blue wet substrate, spreading green moss and brighter plant-like shoots, with 1–3 high-contrast magenta/white mite markers moving independently above the material field.

**Autonomous loop:**

- moss accumulates bounded growth energy only when moisture is nearby;
- growth energy can reinforce existing biomass, spread laterally into wet neighboring space, or extend shoot-like cells against projected gravity;
- water remains ordinary shared mobile material, so board tilt changes moisture distribution and therefore future growth opportunity;
- two mites start active in a fixed three-slot agent array;
- mites deterministically seek nearby moss, wander through safe non-liquid cells, consume biomass, gain energy from feeding, lose energy over time and starve when food disappears;
- periodic bounded rain provides autonomous moisture so the ecology remains active without touch.

Mites are explicit model-owned agents rather than fake material cells. Their position/energy/active state and the scene growth-energy reservoir participate in state hashing because they affect future simulation. Rendering overlays them after 16x16 -> 8x8 aggregation so they remain visible without replacing the world cell under them.

**Inputs:** slider produces a bounded rain pulse around the selected X; combo wakes/spawns an available mite or, if all mite slots are active, seeds moss near water; strong shake/tap/motion scatters active mites under the event budget. Independent A/B touch zones remain disabled.

**Automated evidence:** moisture prerequisite, bounded growth, feeding and energy gain, starvation, deterministic paths/state hashes, tilt-dependent future ecology, independent agent motion, overlay visibility, render persistence and bounded touch/motion events are all host tested.

## Scene 5 — Tracer / Dissolution Plume — future

**Visual identity:** localized concentration source blooming into a carrier fluid with strong concentration-dependent colour change.

**Required behavior:** concentration advection/diffusion, visible plume transport and eventual dilution rather than arbitrary colour cycling. Any touch mapping must continue to use the accepted slider/combo semantics rather than unsupported A/B zones.

## Scene 6+ candidates

After profiling the implemented scenes, choose by maximum new visible behaviour per implementation complexity: ice/melt/refreeze, volcano/pressure vent, smoke/thermal convection, fantasy corrosion, seed/fire/regrowth, or a carefully bounded "everything box".

## Scene order and persistence — ES-009

The stable product order is:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

Cold boot starts at Lava + Water. Short BOOT resets the current scene at exactly its current seed. Long BOOT advances one entry and increments the deterministic seed before initializing the next scene. Scene selection is not persisted across reboot in v0 yet.

## Content quality bar

A scene is not complete merely because named materials exist. It should pass four tests:

1. its large-scale composition is recognizable on the physical 8x8 matrix without serial output;
2. tilt or motion changes behavior in a causally understandable way;
3. it has an autonomous arc such as contact/depletion/recovery/growth rather than decorative looping;
4. the physical perimeter is available to content rather than consumed by invisible implementation scaffolding.

Automated tests can prove deterministic state, bounds, scene distinction and work limits; subjective physical readability remains a board-validation question.
