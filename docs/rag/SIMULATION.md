# ESPsand v0 simulation model

## Internal resolution and world bounds

The deterministic world is **16x16 logical cells** (256 fixed row-major cells). ES-005 maps each logical 2x2 block to one 8x8 physical LED.

World access remains bounds checked. A transport attempt outside 0..15 is rejected, so physical containment is inherent in the finite array. ES-009 therefore removes the redundant explicit wall ring from all product-scene initializers. Non-product fixtures retain their authored walls because those layouts are regression substrates.

This means product edge cells are ordinary simulation space; wall material is not required to define the board edge.

## Cell/state model

The common `Cell` remains the 8-byte ES-004 value:

```text
MaterialId material
uint8      mass
int8       motion_x
int8       motion_y
int16      temperature
uint8      aux
uint8      flags
```

World iteration/hash order is deterministic. Material accounting still reports counts, masses, occupancy and invalid-state diagnostics.

## Material registry and shared dynamics

Stable material IDs remain empty, wall, crust, water, oil, lava, steam, smoke, fire, sodium-like, tracer and moss.

`kMaterialDynamics` remains the one source of transport kind, stylized density, gravity/lateral mobility, thermal conductivity, ambient loss and blocking semantics. Key identities remain water density 120/high mobility, oil density 80, lava density 150/slow, sodium-like density 180, buoyant steam/smoke/fire, and static blocking crust/wall.

These are game-model parameters, not physical measurements.

## Determinism and model lifecycle

`Model` owns PCG32 state, scene, tick, event/reaction budgets and all future-affecting scene state. Hardware timing, wall-clock jitter and unrecorded sensor noise are not model state.

Available scene IDs now include:

- `kDeterminismFixture`
- `kDynamicsFixture`
- `kLavaWater`
- `kSodiumWater`
- `kOilFire`
- `kMossGarden`

The stable product order is Lava -> Sodium -> Oil -> Moss -> Lava. Older scene IDs are not renumbered.

## Work bounds

`WorkBudget` still caps explicit scene events and reaction work per tick. Shared transport/heat/fire passes are fixed scans of the 256-cell world. Scene injections are bounded single attempts and never create queues.

Moss Garden adds no dynamic container or unbounded search structure. It owns a fixed three-slot mite array, performs fixed-capacity scans, caps successful growth actions per growth pass, and uses the existing event budget for user-triggered rain/seed/scatter events.

## Hash/schema boundaries

State hashing remains versioned FNV-1a 64 over explicit canonical fields and is a replay/regression identity, not cryptographic integrity.

The original ES-004 exact golden trace remains unchanged.

Dynamic/product scenes include the shared dynamics schema plus their own scene schema. ES-009 intentionally increments the first three product scene schema versions because their deterministic initial compositions changed:

- Lava + Water: schema 2
- Sodium-like + Water: schema 2
- Oil + Fire: schema 2
- Moss Garden: schema 1

For Moss Garden, model hashing additionally includes growth energy, active mite count, and each fixed mite slot's x/y/energy/active state. These values are hashed because they change future ecology. Render-history persistence is deliberately excluded because it cannot affect future model state.

## Shared transport, heat and reactions

`DynamicsEngine` is unchanged as the common material solver:

- gravity-directed whole-cell transport;
- density swaps and lateral relaxation;
- buoyant gas/fire motion;
- bounded motion disturbance;
- pairwise integer heat exchange plus ambient loss;
- centralized lava+water, sodium-like+water and oil+fire reactions;
- bounded reaction impulse;
- finite fire -> smoke lifetime.

Whole-cell transport and shared reactions preserve participating mass. Explicit scene replenishment/growth is documented scene-owned state creation.

## ES-009 re-composition of chemistry scenes

The shared physics has not been replaced; only deterministic starting/source geometry changed for physical readability.

### Lava + Water

Water now forms a coherent full-width lower basin. Lava starts as a central four-cell-wide source with an early central contact path. Autonomous lava arrives through central vent positions; water refill targets the lower basin. This removes perimeter walls and makes the causal path "hot source falls into blue basin -> crust/steam" larger on the 8x8 display.

### Sodium-like + Water

Water forms a full-width lower pool. Sodium-like reactant uses paired logical cells for several initial/autonomous drops so it survives 2x2 downsampling as a recognizable feature. Contact/reaction/fire/steam/impulse remain shared dynamics.

### Oil + Fire

Oil forms a broad full-width layer above a thinner water layer. Initial fire starts from the left edge so propagation can read as a directional front. Fuel depletion, extinction, refill and re-ignition keep the existing finite cycle semantics.

All three product scenes now contain zero wall mass unless wall is intentionally added as future content.

## Moss Garden — ES-009

Moss Garden adds bounded ecology around the shared water solver.

### Initial state

- water occupies the lower three full-width logical rows;
- moss begins as two substantial wet patches plus two higher-energy shoot cells;
- two mite agents are active in a fixed three-slot array;
- initial growth energy is bounded and deterministic from reset state.

### Moisture and growth

A moss cell is considered locally wet when shared water exists within a small Manhattan neighborhood. At fixed charge intervals, wet moss contributes to a bounded growth-energy reservoir (maximum 512).

At fixed growth intervals, at most three successful growth actions occur:

1. a sufficiently mature/wet moss cell may grow a shoot into empty space **against projected gravity**;
2. otherwise growth may spread into the best neighboring empty cell with moisture support;
3. otherwise a wet existing moss cell may consume growth energy to increase biomass/aux state.

No moisture means no growth/reinforcement. Water itself still moves only through shared `DynamicsEngine`, so tilt changes later moisture opportunity rather than directly setting plant positions.

### Mites

Each mite has only `x`, `y`, `energy`, and `active` state.

- active mites lose energy over time;
- they seek nearest moss deterministically, with model-PRNG fallback wandering;
- they refuse steps into water/lava/oil/fire;
- standing on moss at feed cadence reduces moss mass/aux and increases mite energy;
- depleted moss may disappear to empty;
- energy reaching zero deactivates the mite;
- active count is recomputed from the fixed array and checked by model invariants.

Mites are not material cells. They can therefore feed on/stand over biomass without replacing the underlying world cell.

### Autonomous and external events

- bounded autonomous rain occurs periodically;
- slider touch can inject a small rain pulse near selected X;
- combo first tries to activate/place an inactive mite on moss, otherwise seeds one moss cell near water;
- sufficiently strong motion consumes event budget and performs a bounded scatter attempt for active mites.

## Presentation state is not simulation state

ES-009 temporal persistence is a deterministic RGB-frame blend in runtime/render presentation. It is reset on scene reset/change and does not feed into `World`, scene state, PRNG or hashes. The same model trace is therefore independent of display persistence settings.

## Automated ES-009 evidence

The new host suite proves:

- all product scenes initialize with zero wall material;
- deterministic Moss Garden initialization with water/moss/two active mites;
- no growth when all moisture is removed;
- bounded growth in a wet habitat;
- feeding reduces biomass and increases mite energy;
- mites starve/deactivate without biomass while invariants remain valid;
- a 500-tick same-seed/input duplicate trace keeps identical hashes;
- opposite gravity directions produce different future moss ecology;
- mite coordinates move independently of world cells;
- mite overlay survives 16x16 -> 8x8 projection;
- temporal persistence is deterministic/bounded;
- rain/scatter inputs stay within event budget.

Existing Lava, Sodium, Oil, renderer and shared-dynamics suites continue to pass after the re-composition.

## Safety semantics

Sodium-like and Oil + Fire remain stylized simulation content only. No real reactive-material, fuel, ignition or handling instructions belong in these scene contracts.
