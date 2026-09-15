# ESPsand v0 scene catalogue

Scenes are curated demonstrations built on shared material/reaction primitives. On an 8x8 matrix the quality bar is macroscopic readability **without sacrificing too much spatial information to giant same-colour blocks**.

## Product boundary policy

Product scenes use the 16x16 array bounds as containment and do not reserve a one-cell logical wall ring. All 256 logical positions are available to content, so all **28 physical perimeter LEDs** may participate in animation.

Explicit wall material remains valid for fixtures or deliberately authored obstacles; it is not default scene scaffolding.

## Readability policy — issue #35

Physical feedback after ES-009 established a second failure mode: broad semantic shapes were easier to recognize than the original fine-grained motion, but large rectangular bodies of one material still discarded too much visual information.

Current product composition therefore aims for **structured diversity**:

- irregular shorelines/depths rather than flat slabs;
- thin sources, falling rivulets and sparse drops rather than broad injection blocks;
- ribbons/pockets/fronts rather than multiple solid material rows;
- coherent material bodies with renderer-visible coverage/boundaries rather than random holes/confetti.

The shared physics remains authoritative. Scene policy controls only deterministic starting/source geometry and bounded scene events.

## Scene 1 — Lava + Water — schema 3

**Visual identity:** a thin orange/red lava stream descends through open space toward an irregular blue/cyan water body, producing dark crust and pale/bright steam.

**Starting geometry:** water surface height varies by column and the first one or two depth layers use lower/varied fill mass. Lava follows a roughly two-logical-cell-wide meandering central path rather than a square source block. A seeded near-contact cell still guarantees useful early interaction.

**Autonomous loop:** lava attempts bounded injection through several narrow central vent positions. Water replenishment arrives from staggered top-edge inlet positions as falling rivulets instead of directly painting more basin cells. Shared gravity then decides where those additions settle.

**Inputs:** tilt redirects shared transport; strong motion may perform bounded mass-preserving crust remixing; slider injects a top-edge lava drop near selected X; combo inserts one bounded lava/water contact pair. Independent A/B touch zones remain disabled.

The issue-#35 topology change deliberately increments the scene schema from 2 to **3**.

## Scene 2 — Sodium-like + Water — schema 3

**Visual identity:** sparse pale/warm individual reactant drops over an uneven blue pool, followed by sharp finite fire/steam events.

**Starting geometry:** the pool uses a per-column irregular surface and layered fill masses. Initial sodium-like material is several separated **single logical drops** plus one near-contact drop; the earlier paired/broader-drop composition is removed.

**Autonomous loop:** sodium replenishment inserts one top-edge drop at a time. Slower water refill enters as a falling rivulet from staggered inlet positions. Contact still uses the centralized sodium-like + water reaction and shared bounded reaction impulse.

**Inputs:** tilt controls ordinary transport; shake/tap/motion increase shared mobility; slider inserts one sodium-like top drop near selected X; combo inserts one bounded reactant/water pair.

The issue-#35 topology change increments the scene schema from 2 to **3**. This remains simulation-only content and contains no real reactive-metal procedure.

## Scene 3 — Oil + Fire — schema 3

**Visual identity:** broken amber oil ribbons/pockets floating above irregular blue water, with a sparse left-originating orange/yellow flame front and dim smoke after burn-out.

**Starting geometry:** water depth varies by column. Oil occupies discontinuous surface and secondary ribbon positions rather than several full-width solid rows; cell mass also varies modestly. A small number of left-side oil cells begin as fire so the front has direction without creating a large initial bright block.

**Autonomous loop:** shared oil/fire reactions consume finite fuel and generic fire lifetime produces smoke/extinction. During refill phase, new oil is inserted near the current authored water-surface region one cell at a time; re-ignition preferentially starts at the leftmost existing fuel.

**Inputs:** tilt redistributes fuel through shared transport; shake/motion alter mobility; slider adds one oil top drop near selected X; combo ignites one existing oil cell.

The issue-#35 topology change increments the scene schema from 2 to **3**. This is stylized simulation content, not fuel or ignition guidance.

## Scene 4 — Moss Garden + Mites — schema 1

**Visual identity:** wet blue substrate, spreading green moss/plant shoots and 1–3 high-contrast mite markers moving independently above the material field.

The issue-#35 correction does **not** change Moss Garden model state or scene schema. Its existing ecology remains:

- moisture-gated bounded growth energy;
- lateral wet growth and anti-gravity shoots;
- two initially active mites in a fixed three-slot array;
- deterministic food seeking/wandering, feeding, energy gain/loss and starvation;
- bounded autonomous rain;
- slider rain, combo mite-wake/seed and strong-motion scatter under event budget.

Large wet/green regions are made less visually monolithic by the shared coverage-aware/structural Beauty projection, not by injecting random holes into the ecology.

## Scene 5 — Tracer / Dissolution Plume — future

Future tracer work should preserve the same information-density lesson: localized concentration and plume boundaries should remain readable on 8x8 without becoming either one large flat colour or incoherent random speckle.

## Product order

```text
Lava + Water -> Sodium-like + Water -> Oil + Fire -> Moss Garden -> Lava + Water
```

Cold boot starts at Lava + Water. Short BOOT resets the current scene at the same seed. Long BOOT advances one entry and increments the deterministic seed. Scene selection is not persisted across reboot in v0.

## Content quality bar

A scene should pass five tests:

1. its overall subject is recognizable on the physical 8x8 matrix without serial output;
2. material bodies expose enough edges/coverage/texture that large regions do not collapse into featureless blocks;
3. tilt or motion changes behavior in a causally understandable way;
4. it has an autonomous arc such as contact/depletion/recovery/growth rather than decorative looping;
5. the physical perimeter remains content space rather than hidden implementation scaffolding.

Automated tests enforce determinism, bounds, work limits, projection coverage and coarse blockiness constraints. Subjective physical readability remains a board-validation question.
