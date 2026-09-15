# ESPsand v0 scene catalogue

Scenes are curated demonstrations built on shared material/reaction primitives. On an 8x8 matrix the quality bar is macroscopic readability without filling the display with large same-colour areas.

## Product boundary and sparsity policy

Product scenes use implicit finite-world bounds rather than an explicit containment wall ring. All 16x16 logical positions—and therefore the full 28-LED physical perimeter—remain available to content.

The current physical-tuning baseline adds a stronger content rule:

```text
maximum product material population = 15 logical cells per MaterialId
```

This is intentionally stricter than the requested “less than 16 LEDs of one material” heuristic. One logical cell can affect at most one physical output block, so limiting logical cells to 15 prevents scene-owned material from expanding back into large resting slabs.

Autonomous injection/growth checks the cap before adding material. A 720-tick host regression also checks **every material after every tick** across all four product scenes, including shared reaction products such as crust, steam, fire and smoke.

The cap is a presentation/content contract for current product scenes, not a generic rule imposed on diagnostic fixtures or future intentionally authored obstacles.

## Scene 1 — Lava + Water — schema 4

**Visual identity:** sparse hot lava stream/pockets meeting sparse water pockets, producing dark crust and bright steam.

**Initial state:** approximately 7 lava cells and 12 water cells, distributed through the 16x16 world rather than as a full basin/source block.

**Autonomous loop:** bounded lava vents and much slower water rivulets keep the interaction alive while respecting the 15-cell material ceiling. Gravity, heat, lava/water reaction, steam buoyancy and crust remain shared dynamics.

**Inputs:**

- tilt redirects ordinary shared transport;
- strong motion may perform bounded mass-preserving crust remixing;
- slider spawns **water** near the touched X after the physics pass so the user sees the direct placement before the next gravity tick;
- combo inserts one bounded lava/water contact pair.

## Scene 2 — Sodium-like + Water — schema 4

**Visual identity:** a few pale sodium-like drops crossing a sparse blue water field, followed by local fire/steam events.

**Initial state:** approximately 4 sodium-like cells and 12 water cells.

**Autonomous loop:** sparse sodium-like top drops and slower water rivulets are cap-aware. Contact still uses the centralized sodium-like/water reaction and shared reaction impulse; fire lifetime remains generic.

**Inputs:**

- tilt controls ordinary transport;
- shake/tap/motion increase bounded shared mobility;
- slider spawns **water** near the touched X after physics;
- combo inserts one bounded sodium-like/water contact pair.

This remains stylized simulation content and contains no real reactive-metal procedure.

## Scene 3 — Oil + Fire — schema 4

**Visual identity:** sparse amber fuel pockets, sparse water reference points and a bright local fire front/smoke trail.

**Initial state:** approximately 10 oil/fuel cells and 8 water cells. A short vertical fuel mini-column at the left deliberately guarantees a small real shared-propagation opportunity without restoring a broad fuel layer. The initial fire is created from the lower cell in that column; buoyant transport and the normal oil/fire reaction perform propagation.

**Autonomous loop:** finite fire consumes fuel and expires to smoke. Sparse refill occurs later in the cycle, followed by bounded re-ignition. The quiet/extinguished interval remains part of the scene arc.

**Inputs:**

- slider ignites the nearest existing oil around the touched X after physics, producing local secondary fire state;
- combo performs one bounded existing-oil ignition;
- subsequent propagation and fire lifetime remain entirely in shared dynamics.

This is stylized simulation content, not real fuel/ignition guidance.

## Scene 4 — Moss Garden + Mites — schema 2

**Visual identity:** sparse wet pockets, small moss islands/shoots and high-contrast moving mite markers.

**Initial state:**

- approximately 8 water cells;
- approximately 12 moss cells including two shoots;
- **one** autonomous mite starts active on actual biomass;
- growth energy starts bounded and deterministic.

**Autonomous ecology:**

- moisture gates growth-energy accumulation;
- moss may reinforce, spread locally or extend against projected gravity;
- scene-owned new moss remains capped at 15 cells;
- one autonomous mite seeks/eats biomass, gains/loses energy and may starve;
- slow cap-aware rain maintains moisture opportunity.

**Inputs:**

- slider spawns **water/rain** near the touched X after physics;
- combo seeds a bounded moss cell near moisture;
- strong motion scatters active mites under event budget.

Mites are explicit bounded model agents rather than fake material cells. Up to three fixed mite slots exist, but only one starts active; touch no longer changes agent count.

## Direct touch philosophy

The coarse pinch-slider is no longer a quasi-parameter control. Across the catalogue it means approximately:

> introduce the scene's secondary interactive material/event where I am touching.

Mappings are deliberately scene-specific but spatially consistent:

- Lava → water;
- Sodium → water;
- Oil → fire/ignition;
- Moss → water/rain.

Direct slider actions run after the shared physics pass and before rendering. The new state therefore appears near the selected X for the rendered frame before the next physics tick can move/react it.

Independent A/B touch zones remain unsupported.

## Scene order

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

Cold boot starts at Lava + Water. Short BOOT resets the current scene at the same seed. Long BOOT advances one entry and increments the deterministic seed.

## Luminance identity

All scenes use the shared Beauty contract:

- ordinary/non-energetic material is kept in 0..127, visually centred around the low ~63 region;
- 128..255 is reserved for sparse pseudo-HDR fire/hot lava/hot steam/reaction/agent accents;
- every final frame remains behind the centralized aggregate-load limiter.

## Content quality bar

A product scene is not complete merely because named materials exist. It should satisfy:

1. no material population grows into a large resting slab under the current product cap;
2. scene identity remains recognizable from sparse topology and colour/state contrast;
3. tilt/motion changes behaviour causally;
4. it has an autonomous arc rather than a prerecorded decorative loop;
5. direct touch has a visible spatial consequence near the user's touched X;
6. energetic highlights are sparse and meaningful rather than a general brightness increase;
7. full perimeter remains content space rather than hidden implementation scaffolding.

Automated tests can prove deterministic state, population bounds, replay, code-domain brightness rules and work budgets. Final physical readability and comfort remain board-validation questions.

## Future scenes

Tracer/Dissolution Plume remains the next planned breadth milestone. Future scene authors should start sparse and add activity over time/input rather than treating large pre-filled regions as a readability shortcut.
