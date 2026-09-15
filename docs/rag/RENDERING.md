# ESPsand v0 rendering and LED-output policy

## ES-005 baseline

ES-005 implements the first deterministic logical-world renderer and centralized physical-output budget.

The pure renderer lives under `lib/espsand_core/` and contains no Arduino/ESP dependencies. It maps the fixed ES-004 16×16 world to the physical 8×8 frame by aggregating exactly 2×2 logical cells per LED in deterministic row-major order.

The board-facing `MatrixOutput` remains the only physical NeoPixel gateway. Every frame passed to it, including pre-existing diagnostic frames, is processed by the same pure `OutputLimiter` before the hardware brightness value is applied.

## 16×16 -> 8×8 aggregation

Beauty rendering uses two signals together rather than majority-material voting:

1. **mass-weighted colour** across the four logical cells in each output block;
2. an explicit **importance accent** chosen deterministically from the highest-priority cell in that block.

The accent weight rises for high-priority/emissive materials and for positive temperature/energy. This deliberately allows a small hot/reaction/steam/tracer/biomass feature to remain visible when surrounded by a larger amount of an ordinary material.

The current style priority ordering is intentionally simple and centralized. Fire and hot lava receive the strongest preservation, followed by sodium-like/tracer, steam/moss, then ordinary solids/liquids/gases. These priorities are renderer policy rather than simulation behaviour and can be tuned without introducing scene-local LED writes.

A host fixture locks the important case where one low-mass fire cell shares a 2×2 block with three full water cells: the output must remain visibly warmer/brighter than the all-water block and the renderer records that a minority feature was preserved.

## Material shading and logical energy

The renderer owns one centralized material-style table. Base identities follow the project colour language:

- water: blue/cyan;
- lava/crust: red/orange/dark warm;
- oil: dim amber;
- steam: pale cool;
- smoke: dim neutral/purple-grey;
- fire: yellow/orange hot emission;
- sodium-like: pale warm particle;
- tracer: high-contrast green/yellow family with `aux` concentration modulation;
- moss: green with `aux` modulation;
- wall: neutral grey.

Positive `Cell::temperature` adds bounded warm logical emission before tone mapping. This is a visualization contract only; ES-005 does not implement heat transport or reaction physics.

Unknown material IDs never index outside the style table. They use a deterministic dim-magenta diagnostic fallback and increment `RenderStats::unknown_material_cells`.

## Deterministic tone mapping

Rendering uses integer/fixed-point arithmetic so output does not depend on host floating-point behaviour.

`RenderConfig::exposure_q8` is a Q8 exposure multiplier with 256 = 1.0. Each linear channel is exposed and passed through a fixed rational shoulder:

```text
mapped = exposed * 255 / (exposed + 1024)
```

The result is a bounded 8-bit `Frame8x8`. ES-005 intentionally adds no temporal noise, random dithering or hidden time source. Fixed world state + fixed render configuration therefore gives byte-identical output.

## Diagnostic render modes

`WorldRenderer` exposes deterministic development views:

- `kBeauty` — normal material/energy shading and minority preservation;
- `kMaterialId` — centralized base material identity;
- `kTemperature` — block maximum positive/negative temperature/energy visualization;
- `kMass` — average 2×2 fill/mass visualization.

These are pure render modes usable by host tests or later runtime diagnostics. They do not change world state.

## Centralized output budget

`OutputLimiter` is separate from material shading. It receives the final 8×8 RGB frame plus requested global brightness and computes one `OutputDecision`.

The decision is deterministic and records:

- requested brightness;
- applied brightness;
- raw RGB channel sum;
- estimated aggregate post-brightness frame load;
- whether the hard brightness ceiling limited the request;
- whether the aggregate load envelope limited the request.

The load estimator is deliberately dimensionless:

```text
frame_channel_sum = sum(r + g + b) over 64 pixels
estimated_load = round(frame_channel_sum * applied_brightness / 255)
```

It is **not** a claim about milliamps, battery draw, junction temperature or a certified safe electrical limit.

### Provisional defaults

Until physical current/thermal validation exists, the normal board gateway uses:

- hard brightness ceiling: **32 / 255**;
- aggregate frame-load limit: **4096 software load units**.

The 4096 value is a conservative software envelope, not measured hardware data. It has the useful property that a sparse high-contrast feature can still use the 32/255 development ceiling, while a dense 64-pixel full-white frame is reduced further. Under the current deterministic formula, full-white requested at 255 is limited to **21 / 255**.

A zero load budget fails dark for a non-empty frame. `MatrixOutput::set_output_policy()` may lower/tune policy, but its brightness ceiling is clamped to the current board development ceiling so runtime code cannot raise physical brightness above the unvalidated hard cap.

## Physical-output boundary

No scene owns an `Adafruit_NeoPixel` instance or a GPIO number. `src/board/MatrixOutput` owns the physical LED driver and always performs:

1. pure output-budget decision;
2. hardware global-brightness application;
3. physical RGB-order write;
4. `show()`.

This applies equally to diagnostics and future product scenes. Renderer code can remain entirely host-testable.

## Physical validation still required

ES-005 does **not** promote either default to a measured safe-current or safe-temperature rating.

Before raising the hard ceiling, run a dedicated board validation using representative sparse scenes plus deliberately dense RGB/white stress frames. Prefer an inline USB current meter and temperature probe if available. Exercise candidate ceilings in small increments, allow a sustained soak at each setting, watch for resets/USB instability/visible colour shift, and stop on undesirable heating. Record ambient conditions, pattern, requested/applied brightness, measured current/temperature where available, and soak duration.

Do not infer a safe sustained ceiling from a short visual check. The repository's existing hardware policy treats a 30–60 minute or longer soak as the appropriate class of evidence for sustained-output validation.

## Acceptance properties locked by ES-005

Host tests verify:

- fixed world/config -> byte-identical 8×8 output;
- 2×2 mixed-material aggregation preserves a small high-priority fire feature;
- water/lava/moss palette identities remain distinguishable;
- beauty/material/temperature/mass modes are deterministic;
- unknown material IDs are safe and explicitly counted;
- a dense full-white frame is limited below the 32/255 hard ceiling by the aggregate load policy;
- a sparse bright frame can reach the 32/255 ceiling without false load limiting;
- a zero load budget fails dark;
- a randomized full 16×16 world renders repeatably.

Later renderer work may add temporal persistence, agent overlays or carefully bounded dithering, but any such feature must preserve deterministic replay for an explicit input/time state and must not bypass the common output limiter.
