# AGENTS.md — ESPsand autonomous implementation contract

This repository is designed for serial autonomous implementation through GitHub issues.

## Authority order

When working an issue, treat sources in this order:

1. the active GitHub issue and any explicit user instructions on it;
2. repository documentation under `docs/rag/`;
3. existing accepted implementation and tests on the default branch;
4. reasonable engineering judgement.

If two sources conflict, stop and resolve the conflict in the PR rather than silently choosing an interpretation.

## Required issue workflow

For every implementation issue:

1. Read the entire issue and all referenced RAG documents before editing.
2. Inspect the current default branch; do not assume the repository is still in the state described by an older issue.
3. Create a focused branch and implement only the issue scope plus clearly necessary fixes discovered while working.
4. Keep hardware-specific code behind narrow interfaces so the deterministic simulation remains host-testable.
5. Add or update tests for every deterministic behaviour introduced or changed.
6. Run the repository's prescribed formatting, host tests and firmware build checks locally when practical.
7. Open a PR that links the issue and records: design decisions, files changed, tests/checks run, remaining hardware-only validation, and any deviations from the issue.
8. Wait for automated checks. Fix failures rather than bypassing them.
9. Merge the PR only after required automated checks pass. Do not merge red or unknown CI merely because the change looks small.
10. Verify the merge actually landed on the default branch, then close the issue if the PR did not close it automatically.

The issue may explicitly require physical-board evidence that cannot be produced by a remote agent. In that case, automate everything possible, document the exact manual procedure, and leave the issue open or mark the hardware gate clearly rather than fabricating results.

## Engineering constraints

- Target ESPsand v0: **single board only**. Multi-board networking/topology is deferred.
- USB is sufficient power/control for v0. Do not introduce a battery requirement.
- Do not introduce mandatory cloud, Wi-Fi, Bluetooth, phone, browser or desktop dependencies for runtime use.
- Short BOOT press resets the current scene; long press advances to the next scene.
- IMU-derived tilt/gravity and motion are first-class inputs.
- Capacitive sensing is experimental and should degrade gracefully when unavailable or noisy.
- No issue may require physical sodium, flammable fuel, live mites, dyes, UV illumination or other hazardous materials. Such scenes are visual simulations only.
- Avoid sustained maximum LED brightness. Centralize current/brightness policy.
- Avoid blocking delays in the main runtime loop.
- Prefer fixed-size storage and deterministic bounded work in frame-critical firmware paths.
- Preserve deterministic simulation mode with explicit seeds.
- Do not couple scene logic directly to board drivers.

## Toolchain policy

The foundation issue chooses and records the exact firmware toolchain after inspecting the target board. The planning baseline is PlatformIO with an ESP32-S3-capable Arduino/ESP-IDF environment, but the accepted implementation may refine this if the actual board requires it.

Pure simulation/model code should compile and test on the host where practical. Hardware drivers should have fakes/stubs sufficient to exercise orchestration without physical hardware.

## Documentation policy

Update documentation when implementation resolves an explicit uncertainty in the RAG pack, especially:

- exact board/product revision;
- LED data GPIO and electrical assumptions;
- QMI8658C I2C addresses/pins/orientation;
- BOOT button GPIO and active level;
- free touch-capable GPIOs and measured capacitive behaviour;
- practical LED brightness/current limits;
- achieved frame/update rates.

Do not silently turn an assumption into a fact.
