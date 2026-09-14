# ESPsand v0 product contract

## One-sentence definition

A USB-powered ESP32-S3 8×8 RGB-board showcase that behaves like a tiny energetic material ecosystem controlled by gravity, motion, one button and optionally broad capacitive touch zones.

## Desired first impression

A person should see the board dangling from USB, pick it up, tilt or shake it, press the one button, and immediately discover that the apparent 8×8 toy contains multiple materially distinct scenes with memorable reactions.

The showcase should feel alive rather than menu-driven.

## Required controls

- Short BOOT press: reset/reseed the current scene.
- Long BOOT press: advance to the next scene.
- Tilt: continuously reorients world gravity.
- Shake/swing/tap: scene-aware disturbance input.
- Capacitive zone A/B, when feasible on the actual board without additional components: scene-specific material/event injection.
- Ambiguous/common-mode capacitive disturbance may intentionally map to combo/rare events; perfect touch UI reliability is not a goal.

## Required hero content

The v0 content target is:

1. lava + water with steam/crust/cooling;
2. sodium-like reactive material + water with energetic fizz/heat/gas/impulse;
3. oil/fuel + fire with floating fuel, ignition, flame spread and exhaustion;
4. moss/plants + 1–3 mite-like agents with growth, water dependence and nibbling;
5. tracer/dissolution plume inspired by observed dramatic concentration-dependent colour transitions;
6. at least one additional compact scene selected for maximum visual variety after the hero four are stable.

Hazardous real-world chemistry is never part of the project requirements. These are stylized simulations.

## Experience requirements

- Cold boot reaches a visible scene quickly.
- No runtime dependency on Wi-Fi, Bluetooth, cloud services, phone or PC software.
- Scene changes are obvious without text.
- Resetting a scene gives recognizable content but may reseed stochastic details.
- Motion response must remain usable while the board hangs from a USB cable.
- The board must not rely on sustained full-brightness LEDs for spectacle.
- If capacitive input is unusable on the exact hardware, all scenes remain fully demonstrable through button + IMU.

## Non-goals for v0

- Multi-board adjacency/topology or ESP-NOW chunk exchange.
- Physically accurate CFD, combustion, chemistry or ecology.
- General-purpose user-authored material scripting.
- Battery power or enclosure design.
- Touchscreen-like precise capacitive coordinates.
- Networking configuration UI.
- Exact visual parity with desktop CyberSand.

## Success criteria

v0 is successful when:

- at least four hero scenes are independently recognizable on the 8×8 output;
- tilt materially changes flow direction;
- shake/tap produces useful scene reactions without destabilizing the main loop;
- BOOT short/long gestures are robust;
- pure world/reaction logic is deterministic under a fixed seed and host-tested;
- the final firmware can run for an extended soak without watchdog resets, obvious memory growth or stuck states;
- scene rendering remains within a conservative documented LED brightness/current policy;
- serial diagnostics can report scene, frame rate, IMU status, input events and simulation timing for development.
