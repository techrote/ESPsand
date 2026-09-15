# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a world substantially richer than 64 discrete cells while every frame remains inside one centralized output budget.

## Internal-to-display mapping — ES-005 baseline

The 16×16 simulation maps to the physical 8×8 display by deterministic 2×2 logical aggregation. Beauty rendering combines mass-weighted material colour, one deterministic high-importance accent, bounded positive temperature/energy emission, and bounded material-specific `aux` modulation.

The important-minority rule exists so small reaction fronts, hot material and gas features survive downsampling instead of disappearing behind majority coverage. Rendering remains a pure projection: it does not advance model state or consume model PRNG state.

## Centralized material shading

`WorldRenderer` owns the material-style table aligned with stable material IDs. Product scenes reuse that table rather than defining scene-local LED palettes:

- water: strong blue/cyan identity;
- hot lava: high-priority saturated orange/red plus bounded thermal contribution;
- cooled crust: deliberately much darker red/brown than hot lava;
- steam: pale blue/white with high visual priority;
- sodium-like: pale/warm high-contrast particle;
- oil: dim amber/brown fuel distinct from water and flame;
- fire: strongest high-energy yellow/orange accent;
- smoke: dim neutral/purple-grey gas after finite fire expiry;
- other ES-005 identities remain centralized.

Exact values are artistic simulation parameters, not measured spectra or temperatures. Invalid material IDs continue to use deterministic fallback/counting.

## Sparse reaction highlight — ES-007/008

After ordinary beauty rendering, `SceneRuntime` may brighten **one existing hottest steam or fire output pixel only when the just-completed model tick applied a reaction**. The selected position is derived from actual model state, not wall-clock randomness or a decorative unrelated flash.

ES-007 originally used this only for steam readability. ES-008 generalizes the same bounded mechanism to fire so sodium and oil reactions remain legible on the 8×8 matrix without introducing a second rendering path.

The highlight is post-render but pre-output-limit. It remains subject to the same physical `MatrixOutput` brightness/load policy as every other pixel.

## Logical HDR and scene exposure

The ES-005 logical integer HDR/tone-map contract remains:

```text
mapped = exposed * 255 / (exposed + 1024)
```

`RenderConfig::exposure_q8` uses 256 = 1.0. Current product requests are deliberately modest and scene-specific:

- Lava + Water: exposure Q8 **320**, brightness request **28/255**;
- Sodium-like + Water: exposure Q8 **340**, brightness request **26/255**;
- Oil + Fire: exposure Q8 **300**, brightness request **28/255**.

These values tune contrast/legibility only. They do not bypass the physical limiter.

## Centralized physical output budget

`MatrixOutput` owns the only physical NeoPixel driver and calls `OutputLimiter` for every frame. The provisional board limits remain:

- hard brightness ceiling: **32/255**;
- aggregate frame-load limit: **4096 dimensionless software load units**.

Dense frames may be reduced below the scene request by the aggregate-load limiter. No scene/runtime may raise the board ceiling or call the LED driver directly.

These are conservative development settings, not certified current/temperature ratings.

## Visual identity of the three product scenes

The current content deliberately relies on state/palette contrast rather than full-frame brightness:

- **Lava + Water:** blue/cyan reservoir, hot orange/red viscous material, persistent dark crust, pale steam;
- **Sodium-like + Water:** predominantly blue water with sparse pale particles and sharp finite steam/fire events;
- **Oil + Fire:** dim amber oil above water, bright finite flame fronts, and dim smoke after extinction.

ES-008 host tests require the three initial rendered frames to differ bytewise. That is deterministic automated evidence of distinct projected states, not a substitute for human visual review on the physical 8×8 board.

## Spectacle without sustained maximum brightness

Prefer sparse local contrast, reaction-derived highlights, moving fronts, cooling trails, embers, finite flames and dark post-reaction states. Avoid dense all-white output or permanent high-energy decorative pixels.

Oil + Fire specifically must communicate depletion/extinction through material state, not by leaving an immortal bright flame. Sodium-like reactions are similarly finite/local rather than a sustained flash loop.

## Diagnostic render modes

`WorldRenderer` still provides deterministic `kBeauty`, `kMaterialId`, `kTemperature` and `kMass` projections. The separate bring-up/diagnostic runtime remains available even though normal firmware boots the product catalogue.

`OutputDecision` exposes requested/applied brightness, raw channel sum, estimated load and limiter flags. Product telemetry reports those values for every active scene.

## Physical validation still required

Software proves output-path centralization and limiter arithmetic, not electrical/thermal safety or subjective scene readability. Physical review should confirm:

- all three scene identities are visually distinct on the actual panel;
- sodium/fire/steam highlights are local and readable rather than dense flashes;
- Oil + Fire visibly gutters out before its autonomous refill/re-ignition phase;
- no pixel/colour-order regression exists;
- telemetry shows frames continuing through requested/applied/load limiter state;
- a 30–60 minute or longer active multi-scene soak causes no undesirable heating, reset, USB instability or obvious colour shift.

Do not raise/certify the hard ceiling from a short visual check. Prefer measured current/temperature evidence if available.

## Automated rendering evidence

Existing ES-005/007 renderer tests remain. ES-008 adds a full-scene projection check requiring Lava + Water, Sodium-like + Water and Oil + Fire to produce distinct deterministic initial 8×8 frames. State-evolution tests separately verify finite reaction/fuel behavior; renderer tests do not substitute for physical visual review.
