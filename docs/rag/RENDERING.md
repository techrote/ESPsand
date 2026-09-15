# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a world richer and more causally legible than 64 independent pixels while every physical frame remains inside one centralized output budget.

## Internal-to-display mapping

The 16x16 simulation maps to 8x8 by deterministic 2x2 logical aggregation. `WorldRenderer` combines mass-weighted material colour, high-importance minority preservation, bounded thermal emission and material-specific compact state.

Product scenes do not reserve a logical wall ring, so all **28 physical perimeter LEDs** may display ordinary content.

### Coverage-aware beauty projection — readability correction #35

Physical review after ES-009 exposed a projection problem: the beauty renderer normalized a 2x2 block by only the occupied mass. A block containing one logical material sample could therefore look almost as full/bright as a block containing all four samples. Thin streams, shorelines and isolated particles visually inflated into large square LEDs.

Beauty rendering now preserves sub-pixel logical coverage after mass-weighted colour calculation:

```text
occupied logical samples in 2x2: 0    1    2    3    4
coverage scale (Q8):             0  136  176  216  256
```

This is deliberately not a linear 25/50/75/100% scale: single-cell features remain visible on the physical panel, but they no longer masquerade as completely filled physical pixels. Material-ID, temperature and mass diagnostic modes retain their own explicit semantics.

High-importance minority accents are blended **after** coverage/structural scaling. Small fire, steam and reaction features can therefore remain readable without making every partial liquid block look fully occupied.

## Deterministic structural contrast — readability correction #35

Even a physically correct liquid eventually settles into contiguous regions. On 8x8, a perfectly flat colour field is difficult to parse, so Beauty mode applies a mild deterministic structural scale to water, oil, lava and moss.

The scale derives only from:

- a fixed four-phase `(output x, output y, material ID)` spatial pattern;
- whether adjacent physical blocks have the same dominant material;
- actual logical `motion_x` / `motion_y` proxies in the block.

It does **not** use PRNG draws, timestamps or hidden animation noise. A fixed world always renders identically. Boundaries and moving regions receive slightly more local contrast; stable interiors retain low-amplitude spatial grain rather than collapsing into one exact RGB value.

This treatment occurs before minority accents and before runtime temporal persistence. It is presentation only and cannot affect simulation state or replay hashes.

## Centralized material shading

One shared style table remains authoritative. Important identities include blue/cyan water, orange/red hot lava, dark crust, pale steam, pale/warm sodium-like material, amber oil, bright finite fire, dim smoke and green moss.

Moss uses existing aux modulation for biological variation. Exact RGB values are artistic model parameters, not physical spectra. Invalid material IDs still use deterministic fallback/counting.

## Scene composition for information density

The first ES-009 readability pass made scene shapes larger but physical feedback showed that some of those shapes became excessively homogeneous. Issue #35 replaces broad slabs with coherent but broken-up geometry:

- **Lava + Water:** irregular water shoreline/depth plus a thin meandering lava stream; autonomous water arrives as falling edge rivulets rather than directly extending a basin slab.
- **Sodium-like + Water:** uneven pool with sparse single reactant drops and falling water rivulets rather than paired/broad blocks.
- **Oil + Fire:** broken oil ribbons and pockets floating above an uneven water surface, with a sparse left-originating ignition front rather than several solid rows.
- **Moss Garden:** model layout remains unchanged in this pass; coverage-aware projection and deterministic structural shading break up its large wet/green areas without changing ecology state or hashes.

The intent is **structured diversity**, not confetti: large-scale scene meaning should survive while exact same-colour rectangles become uncommon.

## Temporal presentation persistence — ES-009

`scene_effects::blend_with_previous` blends each base rendered pixel with the previous base frame using fixed integer scene-specific weights. It reduces 60 Hz cell-churn/flicker and helps motion read as continuity.

Current previous-frame weights are:

- Lava + Water: 56;
- Sodium-like + Water: 48;
- Oil + Fire: 64;
- Moss Garden: 96.

History is runtime presentation state only. It resets on scene reset/change and never feeds back into `Model`, PRNG, transport, reactions, agents or state hashes.

The smoothed base frame is saved before transient overlays, so reaction highlights and mites do not leave artificial trails.

## Sparse reaction highlight and mite overlay

On a tick with an applied shared reaction, runtime may brighten one hottest actual steam/fire output position. Moss Garden projects active mite coordinates to `(x/2,y/2)` and raises that output pixel to a bounded high-contrast marker.

Both effects are applied after base persistence and before physical output limiting, so they stay crisp but cannot bypass the common power envelope.

## Logical HDR and scene requests

The ES-005 rational tone-map remains:

```text
mapped = exposed * 255 / (exposed + 1024)
```

Current requested settings remain:

- Lava + Water: exposure 320, brightness 28/255;
- Sodium-like + Water: exposure 340, brightness 26/255;
- Oil + Fire: exposure 300, brightness 28/255;
- Moss Garden: exposure 300, brightness 26/255.

## Centralized physical output budget

`MatrixOutput` remains the only physical NeoPixel gateway. Every frame passes through `OutputLimiter` after renderer, persistence and overlays.

Provisional board policy remains:

- hard brightness ceiling: 32/255;
- aggregate frame-load limit: 4096 dimensionless software load units.

These are conservative development settings, not certified current/temperature limits.

## Diagnostic render modes

`WorldRenderer` retains deterministic Beauty, Material ID, Temperature and Mass projections. The separate bring-up target remains available for raw hardware diagnostics.

## Automated readability evidence

`test_readability` specifically locks properties that can be established without pretending CI can judge aesthetics:

1. a one-of-four occupied logical block must project dimmer than a four-of-four block;
2. a completely uniform 16x16 water world still renders deterministically with at least four non-black physical RGB values and no identical horizontal/vertical run longer than four LEDs;
3. all four product initial frames contain at least six non-black RGB values and no identical non-black run longer than four LEDs;
4. after 180 downward-gravity ticks, all product frames retain at least four non-black RGB values and no identical run longer than five LEDs.

These guards prevent a return to obvious large featureless blocks. They do not prove the result is attractive or immediately understandable.

## Physical validation after readability correction #35

After flashing the corrected `main`, assess specifically:

- thin streams/shorelines now look thinner than fully filled regions;
- settled water/oil/moss bodies have enough internal structure to show shape without looking noisy;
- Lava reads as a narrow hot stream meeting an irregular water body;
- Sodium drops remain individually trackable before reaction;
- Oil reads as broken fuel ribbons/pockets and a spreading/depleting fire front;
- Moss remains coherent while large green/blue regions are less monolithic;
- structural grain does not resemble arbitrary twinkling because it is world-locked;
- temporal persistence still helps continuity without excessive smear;
- reaction flashes and mites remain crisp;
- all effects continue through limiter telemetry.

Subjective readability remains a physical-board judgement. Do not infer electrical/thermal safety from visual comfort; retain the existing multi-scene soak before changing brightness policy.
