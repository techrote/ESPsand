# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a world substantially richer than 64 discrete cells while every frame remains inside one centralized output budget.

## Internal-to-display mapping — ES-005 baseline

The 16×16 simulation maps to the physical 8×8 display by deterministic 2×2 logical aggregation. Beauty rendering combines mass-weighted material colour, one deterministic high-importance accent, bounded positive temperature/energy emission, and bounded material-specific `aux` modulation.

The important-minority rule exists so small reaction fronts, hot material and gas features survive downsampling instead of disappearing behind majority coverage. Rendering remains a pure projection: it does not advance model state or consume model PRNG state.

## Centralized material shading

`WorldRenderer` owns the material-style table aligned with stable material IDs. ES-007 tunes the shared Lava + Water identities rather than adding a scene-local second palette:

- water: stronger blue/cyan identity;
- hot lava: high-priority saturated orange/red plus bounded thermal contribution;
- cooled crust: deliberately much darker red/brown than hot lava;
- steam: pale blue/white and higher visual priority so small gas features survive aggregation;
- fire remains the strongest high-energy accent;
- other ES-005 material identities remain centralized.

Exact values are artistic simulation parameters, not measured spectra or temperatures. Invalid material IDs continue to use deterministic fallback/counting.

## ES-007 reaction highlight

After ordinary beauty rendering, `SceneRuntime` may brighten **one existing hottest steam output pixel only when the just-completed model tick applied a reaction**. The selected position is derived from actual model steam state, not wall-clock randomness or a decorative unrelated flash.

This sparse highlight is intentionally post-render but pre-output-limit. It therefore increases reaction readability on 8×8 while remaining subject to the same physical `MatrixOutput` brightness/load policy as every other pixel.

## Logical HDR and scene exposure

The ES-005 logical integer HDR/tone-map contract remains:

```text
mapped = exposed * 255 / (exposed + 1024)
```

`RenderConfig::exposure_q8` uses 256 = 1.0. ES-007 Lava + Water currently requests Q8 exposure **320** (1.25× logical exposure) to make the first hero scene legible at the deliberately low physical LED brightness.

Exposure is not a bypass: the final physical output limiter is applied afterward.

## Centralized physical output budget

`MatrixOutput` owns the only physical NeoPixel driver and calls `OutputLimiter` for every frame. The current provisional board limits remain:

- hard brightness ceiling: **32/255**;
- aggregate frame-load limit: **4096 dimensionless software load units**.

ES-007 requests global brightness **28/255**. Dense frames may be reduced below that by the aggregate-load limiter. Neither the scene nor runtime may raise the board ceiling or call the LED driver directly.

These are conservative development settings, not certified current/temperature ratings.

## Spectacle without sustained maximum brightness

Lava + Water now demonstrates the intended visual strategy:

- high local contrast between dark crust and hot lava;
- sparse reaction-derived bright steam highlight;
- moving hot/cold fronts produced by state evolution;
- minority preservation so small steam/lava features remain readable;
- persistent dark solid left after reaction rather than full-frame flashing.

Future scenes should continue to prefer sparse local contrast, moving fronts, cooling trails, embers and bounded pulse events over dense all-white output.

## Diagnostic render modes

`WorldRenderer` still provides deterministic `kBeauty`, `kMaterialId`, `kTemperature` and `kMass` projections. The separate bring-up/diagnostic runtime remains available even though normal firmware now boots the product scene.

`OutputDecision` exposes requested/applied brightness, raw channel sum, estimated load, and limiter flags. ES-007 product telemetry reports those values once per second for physical validation.

## Physical validation still required

The software proves output-path centralization and limiter arithmetic, not electrical/thermal safety or subjective scene readability.

For ES-007 physical validation, confirm:

- blue/cyan water, hot orange/red lava, dark cooled crust, and pale/bright steam are visually distinguishable on the actual matrix;
- no pixel/colour-order regression exists;
- reaction highlight is local/sparse rather than a full-frame flash;
- telemetry shows every product frame continuing through the requested/applied/load limiter path;
- a 30–60 minute or longer active-scene soak causes no undesirable heating, reset, USB instability or obvious colour shift.

Do not raise/certify the hard ceiling from a short visual check. Prefer measured current/temperature evidence if available.

## Automated rendering evidence

Existing ES-005 renderer tests remain. ES-007 additionally constructs separate full 2×2 blocks of water, hot lava, cooled crust and steam and requires:

- water to remain blue-dominant;
- lava to remain red-dominant;
- cooled crust total output energy to be lower than hot lava;
- steam to retain a pale/blue-dominant identity.

The full scene tests independently verify reaction/state evolution; renderer tests do not substitute for physical visual review.
