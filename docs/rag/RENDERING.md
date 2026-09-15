# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a world richer and more causally legible than 64 independent pixels while every physical frame remains inside one centralized output budget.

## Internal-to-display mapping

The 16x16 simulation maps to 8x8 by deterministic 2x2 logical aggregation. `WorldRenderer` combines mass-weighted material colour, high-importance minority preservation, bounded thermal emission and material-specific compact state.

ES-009 changes product composition, not this projection. Product scenes no longer place a logical wall ring on the array edges, so the 2x2 blocks feeding all **28 physical perimeter LEDs** may now contain ordinary animated scene content.

## Centralized material shading

One shared style table remains authoritative. Important identities include blue/cyan water, orange/red hot lava, dark crust, pale steam, pale/warm sodium-like material, amber oil, bright finite fire, dim smoke and green moss.

Moss uses existing aux modulation for variation; plant-like shoot state remains a model flag/aux distinction rather than a new material ID.

Exact RGB values are artistic model parameters, not physical spectra. Invalid material IDs still use deterministic fallback/counting.

## Readability composition — ES-009

Physical feedback showed that technically correct sub-cell motion was too difficult to interpret on 8x8. ES-009 therefore moves the first three scenes toward larger coherent shapes:

- Lava + Water: narrow central source against a broad lower basin;
- Sodium-like + Water: paired logical reactant drops against a broad blue pool;
- Oil + Fire: broad amber layer plus a left-originating flame front;
- Moss Garden: large green wet patches plus explicit moving agent markers.

These are scene-state choices; they do not bypass or fake the common physics.

## Temporal presentation persistence — ES-009

`scene_effects::blend_with_previous` blends each base rendered pixel with the previous **base** frame using fixed integer scene-specific weights. Purpose: reduce 60 Hz cell-churn/flicker and make motion read as movement rather than unrelated frame-to-frame pixel changes.

Current previous-frame weights (Q8-like 0..255 scale) are:

- Lava + Water: 56;
- Sodium-like + Water: 48;
- Oil + Fire: 64;
- Moss Garden: 96.

This history is runtime presentation state only. It is reset on scene reset/change and never feeds back into `Model`, PRNG, transport, reactions, agents or state hashes.

The runtime saves the smoothed **base** frame before applying transient overlays. Therefore reaction flashes and mites do not leave artificial trails in subsequent frames.

## Sparse reaction highlight

On a tick with an applied shared reaction, runtime may brighten one hottest actual steam/fire output position. The position derives from live model state. The effect is applied after persistence but before physical limiting, so it stays crisp yet cannot bypass the central power envelope.

## Mite overlay — ES-009

Mites are explicit model agents rather than material cells. After base rendering/persistence, active mite coordinates are projected by integer division from logical `(x,y)` to physical `(x/2,y/2)` and raised to a high-contrast magenta/white marker whose intensity is bounded by mite energy.

This keeps 1–3 agents visible after 2x2 aggregation without erasing or replacing the moss/water cell underneath them. Host tests lock overlay visibility and bounds.

## Logical HDR and scene requests

The ES-005 rational tone-map remains:

```text
mapped = exposed * 255 / (exposed + 1024)
```

Current requested settings:

- Lava + Water: exposure 320, brightness 28/255;
- Sodium-like + Water: exposure 340, brightness 26/255;
- Oil + Fire: exposure 300, brightness 28/255;
- Moss Garden: exposure 300, brightness 26/255.

These are presentation requests only.

## Centralized physical output budget

`MatrixOutput` remains the only physical NeoPixel gateway. Every frame—including persistence output, reaction highlight and mite overlay—is passed to `OutputLimiter`.

Provisional board policy remains:

- hard brightness ceiling: 32/255;
- aggregate frame-load limit: 4096 dimensionless software load units.

Dense output may be reduced below the requested brightness. These values are development policy, not certified current/temperature limits.

## Diagnostic render modes

`WorldRenderer` still provides deterministic Beauty, Material ID, Temperature and Mass views. The separate bring-up target remains available for raw hardware diagnostics.

## Physical validation after ES-009

CI proves deterministic rendering/persistence/agent projection and the centralized limiter path. It cannot prove that the physical scene is easier to understand.

After flashing ES-009, check specifically:

- perimeter LEDs now participate naturally in water/oil/moss/source motion rather than appearing as a static frame;
- the Lava scene reads as source -> basin -> crust/steam;
- Sodium-like drops are visible before they react;
- the Oil fire reads as a moving/depleting front and still reaches extinction;
- Moss Garden shows stable green biomass plus unmistakable independently moving mite markers;
- temporal persistence reduces jitter without making motion feel laggy or washing out reactions;
- all four scenes remain visibly distinct;
- limiter telemetry remains active and no output path bypasses it.

Continue the existing 30–60 minute or longer multi-scene soak before changing the brightness policy. Do not infer an electrical/thermal rating from visual comfort.

## Automated evidence

Existing renderer and scene suites remain green. ES-009 additionally tests that every product scene has zero containment-wall mass, mite overlay changes the correct downsampled pixel, and temporal persistence is byte-deterministic and bounded.
