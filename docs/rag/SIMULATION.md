# ESPsand v0 simulation model

## Internal world and bounds

The deterministic world remains **16x16 logical cells** with 256 fixed row-major entries. Beauty rendering projects each logical 2x2 block to one physical 8x8 LED.

World access is bounds checked. Product scenes use the finite array itself as containment; they do not reserve wall cells around the perimeter. Explicit `wall` remains a valid material for fixtures or deliberately authored obstacles.

## Cell/state model

The common cell remains the compact ES-004 value:

```text
MaterialId material
uint8      mass
int8       motion_x
int8       motion_y
int16      temperature
uint8      aux
uint8      flags
```

World iteration and hashing remain deterministic. Material totals expose per-material cell count/mass and invalid-state diagnostics.

## Shared material dynamics

`kMaterialDynamics` remains the sole generic source of transport kind, stylized density, mobility, thermal parameters and blocking semantics.

`DynamicsEngine` remains authoritative for:

- gravity-directed transport and density swaps;
- lateral relaxation;
- buoyant gas/fire movement;
- motion disturbance;
- pairwise heat exchange/ambient loss;
- centralized lava+water, sodium-like+water and oil+fire reactions;
- bounded reaction impulse;
- finite fire -> smoke lifetime.

The physical-tuning pass does **not** introduce scene-local replacement fluid/chemistry solvers.

## Sparse product material population

Current product scenes adopt a shared content limit:

```text
kProductMaterialCellLimit = 15
```

`material_cell_count()` and `can_add_product_material()` provide the common scene-policy check.

Scene-owned material creation respects this limit:

- autonomous replenishment;
- touch-local material spawning;
- Moss growth/seeding/rain.

The rule is intentionally expressed in logical cells rather than rendered LED estimates. Because one logical cell can contribute to at most one physical 2x2 output block, 15 logical cells is a conservative implementation of the user's “less than 16 pixels of one material” requirement.

Shared transport/reactions can change identities independently of scene-owned creation, so CI additionally runs all four product scenes for 720 resting/downward-gravity ticks and checks every material after every tick. That regression guards persistent reaction products as well as source material.

This cap is a current product presentation invariant, not a universal physics law for all future fixtures/content.

## Determinism and lifecycle

`Model` still owns:

- scene ID and seed;
- model-owned PCG32;
- tick;
- event/reaction work budgets;
- fixed scene state such as Moss mites/growth energy;
- canonical state hashing.

No core rule uses hidden entropy, `rand()`, timestamps or renderer state.

Product order remains:

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

Fixture IDs remain outside product cycling.

## Product tick ordering — physical-tuning baseline

Product tick order now has an explicit post-dynamics direct-control seam:

```text
sanitize InputFrame
reset bounded work counters
scene.before_dynamics(...)
DynamicsEngine::step(...)
scene.after_dynamics(...)
increment model tick
```

`before_dynamics` owns autonomous/source/lifecycle policy and non-direct events.

`after_dynamics` is reserved for direct spatial slider actions. Its purpose is presentation/interaction timing: a touched secondary material/actor is inserted after the current physics step, so it exists at approximately the touched X in the rendered model state before the next tick is allowed to move/react it.

The new object is ordinary model state. On the following tick it obeys exactly the same shared dynamics/ecology rules as autonomous material.

Fixtures remain unaffected by this seam.

## Lava + Water — schema 4

Sparse initial state uses roughly 7 lava cells and 12 water pockets rather than broad slabs.

- autonomous lava and water replenishment is slower and cap-aware;
- slider post-step action adds water near selected X;
- combo inserts a bounded contact pair;
- crust remix remains bounded/mass-preserving;
- actual reaction/gas/heat/transport remain shared.

## Sodium-like + Water — schema 4

Sparse initial state uses roughly 4 sodium-like drops and 12 water pockets.

- autonomous sodium/water replenishment is cap-aware;
- slider post-step action adds water near selected X;
- combo inserts one bounded sodium-like/water pair;
- shared reaction produces finite fire/steam and bounded impulse.

## Oil + Fire — schema 4

Sparse initial state uses roughly 10 oil cells and 8 water cells.

A short vertical fuel mini-column provides one deterministic shared-propagation opportunity. The initial fire is created by converting the lower fuel cell. Under shared buoyant transport/reaction ordering, fire moves through nearby fuel and the normal oil/fire rule performs propagation; no scene-local chain-reaction solver is added.

Later sparse refill/re-ignition retains the finite burn/extinguish arc.

Slider post-step action ignites nearest existing oil around selected X.

## Moss Garden — schema 2

Sparse initial state uses roughly:

- 8 water cells;
- 12 moss cells including shoots;
- 1 active starting mite.

Growth remains moisture-gated and bounded. New moss cannot cross the 15-cell material ceiling; reinforcement of existing cells can continue without increasing population.

Slider post-step action activates/places an inactive mite on moss near selected X. Combo remains a bounded moss seed event near moisture. Strong motion still scatters mites.

Mite state remains a fixed three-slot array and is included in state hashing because it affects future ecology.

## Hash/schema boundaries

State hashing remains versioned FNV-1a 64 over explicit canonical fields and is a replay/regression identity rather than cryptographic integrity.

The original ES-004 exact golden fixture path remains unchanged.

Current product scene schema versions are:

```text
Lava + Water        4
Sodium-like + Water 4
Oil + Fire          4
Moss Garden         2
```

The increments are deliberate because sparse initial state, touch semantics and/or future-affecting scene behavior changed.

Renderer pseudo-HDR allocation and temporal RGB history are excluded from model hashes because they cannot affect future simulation.

## Work bounds

Existing fixed event/reaction budgets remain authoritative. Direct slider actions consume event budget and retain their cadence gate.

No new dynamic allocation, recursive reaction processing or unbounded queue was introduced. The long-run material population test also checks model invariants while the scenes evolve.

## Safety semantics

Sodium-like and Oil + Fire remain stylized visual/game simulation content. Do not encode real reactive-metal, fuel, ignition, quantity or handling instructions into scene behavior or documentation.
