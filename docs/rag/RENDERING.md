# ESPsand v0 rendering and LED-output policy

## Goal

Make 64 LEDs imply a world substantially richer than 64 discrete cells.

The renderer is responsible for translating a higher-resolution logical world into an expressive 8×8 frame while respecting a centralized brightness/current budget.

## Internal-to-display mapping

Planning baseline: 16×16 simulation -> 8×8 display, normally aggregating 2×2 logical cells per LED.

Do not simply choose the most common material. Preserve visually important minority phenomena such as:

- hot reaction fronts;
- tiny bright embers;
- moving mite agents;
- bubbles/steam;
- thin moss/plant tips;
- high-concentration tracer cores.

Possible aggregation inputs:

- material mass/coverage;
- emissive/thermal energy;
- weighted material priority;
- temporal persistence;
- agent overlay.

The implementation should be simple, deterministic and testable before adding artistic heuristics.

## Logical HDR, physical restraint

Scenes/materials may emit logical RGB values in a wider internal range or use additive contributions. A final output stage owns:

1. exposure/tone mapping;
2. global brightness ceiling;
3. optional approximate current estimator;
4. temporal smoothing/dithering if useful;
5. hardware colour-order conversion.

No scene is allowed to bypass this stage.

## Spectacle without sustained maximum brightness

Prefer:

- one- or two-frame reaction flashes;
- local contrast;
- pre-flash dimming of surrounding pixels;
- fast colour-temperature changes;
- moving fronts;
- embers and sparse sparks;
- pulse envelopes;
- temporal dithering;
- cooling trails;
- steam fade and smoke fade.

Avoid using all-white/full-bright frames as the default way to communicate energy.

## Colour language

Default material identities should be distinct at a glance:

- water: blue/cyan range;
- lava/hot rock: red/orange/yellow, cooling toward dark red/black;
- steam: cool/pale brief emission;
- oil/fuel: dim amber/brownish warm tones before ignition;
- flame: yellow/orange/white core with fast flicker;
- smoke: dim neutral/purple-grey approximation appropriate to RGB LEDs;
- moss/plants: multiple greens with moisture/health modulation;
- mites: contrasting warm/bright points;
- tracer: concentration-dependent multi-hue palette deliberately capable of red/orange -> lime/green -> yellow transitions.

Exact palettes are artistic parameters, not physical measurements.

## Diagnostics render modes

Provide development-only views where practical:

- raw material IDs;
- temperature/energy;
- velocity/motion proxy;
- density/fill;
- touch-zone readings;
- IMU gravity vector indicator;
- frame/power limiter status.

These modes may be selected by compile flag or serial command and do not need to be exposed through the one-button showcase UI.

## Acceptance metrics

Renderer work should demonstrate:

- deterministic output for fixed world state/time inputs;
- no pixel writes outside the 8×8 frame;
- central brightness clamp applied to every scene;
- at least one test for supersampled aggregation of mixed materials;
- no sustained full-brightness output from default scenes under normal operation.
