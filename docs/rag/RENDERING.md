# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a richer, causally legible world while using the panel's strongest perceived dynamic range efficiently and keeping every physical frame behind one centralized load limiter.

## Internal-to-display mapping

The deterministic 16x16 simulation maps to the physical 8x8 display through 2x2 logical aggregation.

Beauty rendering combines:

- mass-weighted material colour;
- explicit 1/4..4/4 logical coverage scaling;
- important-minority preservation;
- bounded temperature/energy contribution;
- deterministic world-locked structural contrast from boundaries, motion and stable coordinates.

The coverage correction is important: one occupied logical subcell no longer projects nearly as strongly as four occupied subcells. Thin streams, particles, shorelines and sparse pockets therefore remain visibly thinner than fully occupied material.

Rendering remains a pure projection. It does not advance the model or consume model PRNG state.

## Ordinary luminance domain — 0..127

Physical-board feedback shows that this LED chain has substantially more useful perceived differentiation at low code values than in the upper portion of 8-bit output.

Beauty mode therefore defines a perceptual ordinary domain:

```text
ordinary centre ~= 63 / 255
ordinary maximum = 127 / 255
```

After the existing logical HDR/tone-map, coverage and structural shading, non-energetic Beauty channels are compressed approximately in half and clamped to 127. This means an old midrange value around 126 maps close to 63, while an old full-scale ordinary value maps to 127.

The values 63 and 127 are **artistic/display code landmarks**, not measured luminance, electrical current or thermal limits.

Ordinary examples include settled water, oil, moss, crust, smoke and other non-hot material state.

## Pseudo-HDR domain — 128..255

Beauty values above 127 are reserved for sparse causal state that benefits from conspicuous contrast. Current eligibility is deliberately narrow:

- fire;
- sufficiently hot lava;
- sufficiently hot steam;
- reaction highlights applied by runtime;
- high-contrast mite overlays.

The base renderer tracks how many physical pixels enter this pseudo-HDR domain through `RenderStats::pseudo_hdr_pixels`.

The upper half is called **pseudo-HDR** because it is only a perceptual/artistic allocation inside an 8-bit LED code range. It is not HDR in a calibrated photometric sense.

## Structural contrast without decorative noise

Water, oil, lava and moss receive low-amplitude deterministic variation based on:

- stable matrix position/material identity;
- same-material neighboring blocks;
- actual logical motion proxies.

No random twinkle, timestamp noise or PRNG draw is added by rendering. A fixed world always produces the same Beauty frame.

The intended balance is coherent material with visible internal structure—not monolithic colour slabs and not random confetti.

## Temporal persistence

`scene_effects::blend_with_previous` still blends each current base frame with the previous base frame using scene-specific fixed integer weights. This reduces 60 Hz cell-churn and makes motion easier to follow.

Current previous-frame weights remain approximately:

- Lava + Water: 56;
- Sodium-like + Water: 48;
- Oil + Fire: 64;
- Moss Garden: 96.

History is runtime presentation state only. It resets on scene reset/change and never feeds into `Model`, PRNG, dynamics or hashes.

Reaction highlights and mite overlays are applied after persistence, so they remain crisp and do not leave artificial trails.

## Product output scalar and centralized physical limiter

The old global product scalar request of roughly 26–28/255 and provisional 32/255 scalar ceiling would collapse the new 0..255 Beauty-domain distinction before it reached the LEDs. Product runtime therefore now requests the full scalar value **255**, and the default scalar ceiling permits 255.

This does **not** mean a dense 255 frame is considered safe.

Every frame still passes through:

```text
WorldRenderer / overlays
 -> MatrixOutput
 -> OutputLimiter
 -> LEDs
```

The mandatory aggregate frame-load envelope remains:

```text
4096 dimensionless post-brightness load units
```

Dense/high-energy frames automatically receive a lower applied scalar when necessary. Sparse pseudo-HDR pixels may reach the full code range when the aggregate frame remains inside that envelope.

The 4096-unit policy is still a conservative software development envelope, not a measured current or temperature rating.

## Sparse population complements pseudo-HDR

The shared product-scene material limit is 15 logical cells per material. This reduces large resting blocks and also makes the upper pseudo-HDR range naturally sparse: hot/reactive state is local rather than a panel-wide brightness field.

CI checks the material population rule over a 720-tick resting run for every product scene.

## Diagnostic modes

Material ID, Temperature and Mass diagnostic projections remain deterministic and are not constrained to the ordinary Beauty-domain 0..127 convention. They are tooling views, not normal product presentation.

## Physical validation required

Automated tests prove code-domain rules and limiter arithmetic. They do not prove subjective brightness quality or electrical/thermal safety.

After flashing this tuning pass, check specifically:

- ordinary water/oil/moss/crust occupy a comfortable low-luminance range with useful gradation around the first ~80 code steps;
- 127 feels like a sensible maximum for non-energetic material;
- fire/hot lava/reaction flashes are clearly brighter without dominating the whole matrix;
- pseudo-HDR remains spatially sparse;
- the aggregate limiter still reports requested/applied/load state and intervenes on dense frames;
- no reset, USB instability, colour shift or undesirable heating appears in a sustained multi-scene soak.

Do not turn the new 255 scalar allowance into an electrical safety claim. Measured current/temperature evidence is still required before certifying or relaxing the aggregate-load policy.

## Automated evidence

Host coverage now requires:

- ordinary Beauty channels stay at or below 127;
- representative ordinary water/oil remain visible in the low range;
- hot fire/lava may exceed 127;
- pseudo-HDR pixel counting is deterministic;
- a sparse one-pixel full-code frame can retain scalar 255 under the default limiter;
- an explicitly dense white frame is load-limited;
- zero load budget still fails dark;
- all existing renderer determinism/palette/coverage tests remain green.
