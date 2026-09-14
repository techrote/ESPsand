# ESPsand v0 simulation model

## Internal resolution

Planning baseline: **16×16 logical world rendered to 8×8 LEDs**. This is deliberately provisional and should be benchmarked early. It gives four logical samples per LED while remaining trivial for the ESP32-S3.

If profiling shows 16×16 is unnecessarily limiting, an implementation may increase resolution provided deterministic host tests and frame budget remain intact. Do not lower to 8×8 merely because the display is 8×8 unless evidence shows supersampling provides no value.

## Cell/state model

Keep the common cell compact. A practical conceptual representation is:

```text
material_id
mass/fill
vx, vy or compact momentum proxy
temperature/energy
aux
flags
```

Not every field must be literally stored per cell if a cheaper representation preserves behaviour. `aux` may be material-specific: fuel, dissolved concentration, biomass, age, etc.

## Material categories

v0 should support enough generic behaviour for:

- empty/gas;
- wall/solid rock/crust;
- water-like liquid;
- oil/fuel-like liquid with lower effective density than water;
- lava/hot viscous liquid;
- steam/smoke/gas;
- fire/ember energy carrier;
- sodium-like reactive solid/particle;
- tracer/dissolved scalar;
- moss/plant biomass;
- mite-like mobile agents, which may live outside the dense cell grid if cleaner.

Material names are expressive game/simulation names, not claims of chemical fidelity.

## Transport

The transport kernel should prioritize recognizable macroscopic behaviour at tiny scale:

- gravity-directed movement;
- density ordering / buoyancy tendencies;
- lateral relaxation for liquids;
- viscosity differences;
- bounded momentum/advection proxy where it materially improves slosh;
- gas rise/dispersion;
- solids that settle and block.

Conservation should be testable for material mass except where reactions explicitly create/remove/transform it.

## Heat

Use a cheap bounded thermal model:

- per-cell/material temperature or energy;
- local diffusion/exchange;
- material-specific heating/cooling rates;
- phase/reaction thresholds where useful;
- ambient loss.

Visuals can exaggerate thermal gradients. Numerical stability matters more than physical units.

## Reactions

Use data-driven or centralized reaction rules rather than scene-specific cross-material conditionals.

Examples:

```text
LAVA + WATER -> cooling lava/crust + steam + heat/impulse
HOT/FIRE + OIL -> flame/heat + fuel consumption + smoke/gas
SODIUM_LIKE + WATER -> heat + gas + impulse + consumed reactant
WATER + PLANT/MOSS -> increased growth budget
MITE + BIOMASS -> consumed biomass + mite energy
```

Every reaction specifies bounded products/effects and participates in a per-tick reaction/event budget. Chain reactions must not recurse without limit.

## Sodium scene safety semantics

The sodium-like material exists only to generate a recognizable energetic simulation. Do not encode procedures, quantities or experimental guidance for handling real sodium or reactive metals. Implementation is purely visual/game physics.

## Tracer scalar

Tracer/plume scenes may model a concentration field separate from bulk water or encode concentration in cell aux state. Desired behaviours:

- localized injection;
- advection with water;
- diffusion/mixing;
- palette mapping that can traverse deep red/orange -> lime/green -> yellow or other dramatic sequences as concentration changes.

This palette is an artistic/observational homage, not a spectroscopic model.

## Biology

Biology is deliberately tiny:

- moss spreads preferentially into wet/supporting neighbours;
- plants can spend a growth budget to extend shoots against gravity and roots with/along moisture;
- 1–3 mite-like agents wander, seek biomass at short range, nibble it, gain energy and can starve;
- optional reproduction is bounded and off by default if it damages clarity.

Biological rules must share the same deterministic seeded PRNG.

## Deterministic verification

Host tests should cover:

- fixed seed -> identical state hash after N ticks;
- mass conservation in transport-only fixtures;
- density ordering/buoyancy tendencies;
- reaction stoichiometry at the game's abstract level;
- heat diffusion boundedness;
- no out-of-bounds state access under randomized stress inputs;
- event/reaction budget saturation behaves predictably.
