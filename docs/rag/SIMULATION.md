# ESPsand v0 simulation model

## Internal resolution and world bounds

The deterministic world is **16x16 logical cells** (256 fixed row-major cells). Each logical 2x2 block maps to one 8x8 physical LED.

World access is bounds checked. A transport attempt outside 0..15 is rejected, so product containment is inherent in the finite array; product scenes do not add a redundant border wall. Non-product fixtures retain any authored walls required by their regression layouts.

## Cell/state model

The common `Cell` remains the compact 8-byte ES-004 value:

```text
MaterialId material
uint8      mass
int8       motion_x
int8       motion_y
int16      temperature
uint8      aux
uint8      flags
```

World iteration/hash order and material accounting remain deterministic.

## Material registry and shared dynamics

Stable material IDs remain empty, wall, crust, water, oil, lava, steam, smoke, fire, sodium-like, tracer and moss.

`kMaterialDynamics` remains the one source of transport kind, stylized density, gravity/lateral mobility, thermal conductivity, ambient loss and blocking semantics. Scene readability work does not create a second fluid solver or material table.

## Determinism and lifecycle

`Model` owns PCG32 state, scene, tick, work budgets and all future-affecting scene state. Hardware timing, rendering history, wall-clock jitter and hidden sensor noise are not model state.

Product order remains:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

Older scene IDs are not renumbered.

## Work bounds

`WorkBudget` caps explicit scene events and reaction work per tick. Shared transport/heat/fire passes are fixed scans of the 256-cell world. Scene injections remain bounded single attempts and never create queues.

Moss Garden owns a fixed three-slot mite array, bounded growth energy and fixed-capacity scans; no readability change introduces dynamic allocation or unbounded agent work.

## Hash/schema boundaries

State hashing remains versioned FNV-1a 64 over explicit canonical future-affecting state. The original ES-004 golden fixture remains unchanged.

Current product scene schema versions are:

- Lava + Water: **3**
- Sodium-like + Water: **3**
- Oil + Fire: **3**
- Moss Garden: **1**

ES-009 created schema 2 for the first chemistry re-composition. Readability issue #35 deliberately advances those three to schema 3 because their deterministic starting/source topology changes again. Moss Garden model state is unchanged by issue #35, so its schema remains 1; only the shared renderer projects its large material regions differently.

Moss hashing still includes growth energy and every fixed mite slot's x/y/energy/active state. RGB coverage/structural shading and temporal presentation history are excluded because they cannot affect future simulation.

## Shared transport, heat and reactions

`DynamicsEngine` remains authoritative for:

- gravity-directed whole-cell transport;
- density swaps and lateral relaxation;
- buoyant gas/fire motion;
- bounded motion disturbance;
- integer heat exchange plus ambient loss;
- centralized lava+water, sodium-like+water and oil+fire reactions;
- bounded reaction impulse;
- finite fire -> smoke lifetime.

Whole-cell transport and reactions preserve participating mass. Explicit scene replenishment and biological growth remain documented scene-owned state creation.

## Structured chemistry topology — readability issue #35

The physical board showed that broad ES-009 slabs still hid too much information. The following are **initial/source geometry changes only**; generic material motion/reactions remain shared.

### Lava + Water

- water has a deterministic irregular per-column surface instead of a flat full-width top;
- surface and near-surface logical cells use lower/varied fill mass while deeper cells remain fuller;
- lava begins as a thin roughly two-cell-wide meandering central stream rather than a square source;
- one seeded lava cell immediately above the local water surface preserves early contact;
- autonomous lava arrives through a bounded central vent set;
- autonomous water enters from staggered top-edge inlet positions as falling rivulets rather than being painted into the settled pool.

This keeps a coherent “hot stream meets water” causal story while producing more partial 2x2 blocks and boundaries for physical rendering.

### Sodium-like + Water

- water uses a different irregular per-column surface/depth profile;
- initial sodium-like material is several separated single logical drops plus one near-contact drop rather than paired/broad drops;
- autonomous sodium insertion is one top-edge drop;
- autonomous water refill is a falling top-edge rivulet;
- combo remains one bounded sodium/water contact pair handled by shared reactions.

The scene still proves finite reactant consumption, fire/steam production and bounded reaction impulse.

### Oil + Fire

- water is a shallow irregular lower reference body rather than fixed full rows;
- oil is authored as discontinuous surface and secondary ribbons/pockets with modest mass variation;
- initial fire converts only a few existing left-side oil cells, producing a directional front without a broad bright block;
- refill inserts one oil cell near the authored water-surface region at a time;
- autonomous re-ignition preferentially selects leftmost existing fuel; touch combo may still ignite one deterministic PRNG-selected oil cell.

Finite fuel, fire expiry, smoke, quiet extinction and refill/re-ignition semantics remain unchanged.

## Moss Garden

Moss Garden model semantics remain the ES-009 baseline:

- water is ordinary shared mobile material;
- nearby water charges a bounded growth-energy reservoir;
- growth may reinforce biomass, spread toward wet neighbors or extend shoots against projected gravity;
- two mites start active in a fixed three-slot array and seek/feed/wander/starve deterministically;
- periodic bounded rain keeps the autonomous ecology active;
- slider rain, combo mite/seed action and strong-motion scatter use the existing event budget.

Issue #35 intentionally does not punch decorative holes into the ecology; visual subdivision of large blue/green regions belongs to Beauty projection.

## Rendering is not simulation

Coverage-aware 2x2 scaling and deterministic structural shading are pure renderer operations. Stable coordinate phase, neighboring dominant-material boundaries and current motion proxies may affect the RGB frame, but no renderer result feeds back into `World`, scene state, PRNG, work budgets or hashes.

Runtime temporal persistence remains presentation state and is also excluded from deterministic model identity.

## Automated evidence

The full model/dynamics/scene suites continue to lock deterministic replay, mass/reaction behavior, finite fire, ecology, tilt response and work limits.

Issue #35 adds separate projection/readability tests rather than weakening these behavioral suites. The only legacy assertions adjusted are initial material-minimum guards whose old values specifically encoded the superseded slab compositions: Lava still requires at least 12k water mass and 2k lava mass; Sodium still requires more than 10k water mass plus finite reactant.

## Safety semantics

Sodium-like and Oil + Fire remain stylized simulation content only. No real reactive-material, fuel, ignition or handling instructions belong in these contracts.
