# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a world substantially richer than 64 discrete cells.

The renderer translates the higher-resolution logical world into an expressive 8×8 frame while respecting a centralized brightness/current budget.

## Internal-to-display mapping — ES-005 baseline

ES-005 locks the initial 16×16 simulation -> 8×8 display mapping: each output LED aggregates exactly one deterministic 2×2 logical block.

The implementation does not simply choose the most common material. Beauty rendering combines:

- mass/fill-weighted material colour across the block;
- one deterministic high-importance accent from the strongest visually important logical cell;
- bounded positive temperature/energy emission;
- bounded `aux` palette modulation for tracer and moss.

The accent rule explicitly preserves visually important minority phenomena. Fire and hot lava have the strongest priority, followed by sodium-like/tracer, steam/moss and ordinary material classes. A host test locks the case where one low-mass fire cell shares a block with three full water cells and still materially changes the output pixel.

Future agent overlays or temporal persistence can extend this projection, but they must remain deterministic from explicit state/time inputs.

## Centralized material shading

`WorldRenderer` owns a single material-style table aligned with the stable ES-004 material IDs. Default identities remain:

- water: blue/cyan range;
- lava/hot rock: red/orange/yellow, cooling toward dark red/black;
- steam: cool/pale brief emission;
- oil/fuel: dim amber/brownish warm tones before ignition;
- flame: yellow/orange/white core;
- smoke: dim neutral/purple-grey approximation appropriate to RGB LEDs;
- moss/plants: greens with bounded `aux` modulation;
- sodium-like: pale warm particle;
- tracer: high-contrast green/yellow family with concentration modulation;
- wall: neutral grey.

Exact palettes are artistic parameters, not physical measurements. Unknown material IDs use a deterministic dim-magenta fallback and increment a renderer diagnostic counter rather than indexing outside the style table.

## Logical HDR, tone mapping and physical restraint

Material shading operates in a wider integer logical range before output. `RenderConfig::exposure_q8` is a Q8 exposure multiplier with 256 = 1.0. Each channel uses the fixed rational shoulder:

```text
mapped = exposed * 255 / (exposed + 1024)
```

No hidden time source, floating-point randomness or temporal noise is involved in ES-005. Fixed world state + fixed render configuration therefore gives byte-identical 8-bit output.

The final physical output stage then owns:

1. requested global brightness;
2. a hard brightness ceiling;
3. deterministic aggregate RGB PWM-load limiting;
4. physical colour-order conversion.

No scene is allowed to bypass this stage.

## ES-005 centralized output budget

The pure `OutputLimiter` receives the final `Frame8x8` and requested brightness. It computes:

```text
frame_channel_sum = sum(r + g + b) over 64 pixels
estimated_load = round(frame_channel_sum * applied_brightness / 255)
```

`estimated_load` is a dimensionless software PWM-load proxy, **not milliamps**.

Until physical current/thermal validation is available, `MatrixOutput` uses:

- hard brightness ceiling: **32/255**;
- aggregate frame-load limit: **4096 software load units**.

The second limit deliberately makes dense output dimmer than sparse highlights. With the current deterministic formula, 64-pixel full-white requested at 255 is reduced to **21/255**, while a single full-white pixel may still reach the 32/255 ceiling.

A zero load budget fails dark for non-empty output. Runtime policy can lower/tune settings, but the board gateway clamps policy so code cannot raise physical brightness above the current unvalidated hard ceiling.

These values are conservative development policy, not a certified safe electrical/thermal rating.

## Spectacle without sustained maximum brightness

Prefer:

- one- or two-frame reaction flashes;
- local contrast;
- pre-flash dimming of surrounding pixels;
- fast colour-temperature changes;
- moving fronts;
- embers and sparse sparks;
- pulse envelopes;
- temporal dithering where deterministic and justified;
- cooling trails;
- steam fade and smoke fade.

Avoid using all-white/full-bright frames as the default way to communicate energy.

## Diagnostic render modes

`WorldRenderer` provides pure deterministic development projections:

- `kBeauty` — material/energy shading plus minority preservation;
- `kMaterialId` — raw centralized material identity palette;
- `kTemperature` — block temperature/energy visualization;
- `kMass` — block fill/mass visualization.

Existing touch and IMU diagnostics remain separate runtime views. `OutputDecision` exposes requested/applied brightness, raw channel sum, estimated load and whether either limiter engaged, providing the seam for later serial/visual limiter diagnostics.

## Physical-output boundary

`src/board/MatrixOutput` owns the only `Adafruit_NeoPixel` object. Every call to `present()` runs through the same `OutputLimiter`, including the existing pixel/colour/gravity/touch diagnostics. Scene code can therefore not accidentally skip the budget by using a different render path.

## Physical validation still required

Before raising or certifying the hard brightness ceiling, run a dedicated board validation using representative sparse scenes plus deliberately dense RGB/white stress frames. Prefer an inline USB current meter and temperature probe if available. Exercise candidate settings in small increments, allow a sustained soak at each setting, record ambient conditions/pattern/applied limiter state/current/temperature where available, and watch for resets, USB instability or colour shift. Stop on undesirable heating.

Do not infer a safe sustained ceiling from a short visual check. The project treats a 30–60 minute or longer soak as the appropriate class of evidence for sustained-output validation.

## Acceptance metrics locked by ES-005

Renderer tests demonstrate:

- deterministic output for fixed world/configuration;
- no pixel writes outside the 8×8 fixed frame;
- important-minority preservation in mixed 2×2 aggregation;
- centralized palette distinctions;
- safe unknown-material handling;
- deterministic material/temperature/mass diagnostic views;
- hard brightness clamping and aggregate-load limiting;
- sparse highlight preservation under the same budget;
- repeatable randomized full-world rendering.

No default or diagnostic path assumes sustained 255/255 all-white output is safe.
