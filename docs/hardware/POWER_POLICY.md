# ESPsand board power / thermal policy

ESPsand v0 is a local, single-board device and does not require Wi-Fi or Bluetooth at runtime. The board also carries 64 addressable RGB LEDs, so avoidable MCU/radio dissipation should not be added to the LED thermal load.

## Startup policy

Both the normal firmware and the minimal hardware bring-up target call the board power policy before starting their application runtime.

The policy:

- leaves Wi-Fi uninitialized when it is already off;
- if any dependency has initialized Wi-Fi, stops it and deinitializes the Wi-Fi driver;
- disables/deinitializes the Bluetooth controller if it is active;
- releases the ESP32-S3 BLE controller memory reservation after the controller is idle;
- keeps native USB Serial/JTAG enabled for development and diagnostics;
- keeps the QMI8658 IMU, watchdogs, brownout protection and PSRAM enabled.

The startup probe reports the result as:

```text
power.policy applied=1 wifi_off=1 bt_off=1 bt_mem_released=1 wifi_err=0 bt_err=0
power.keep usb_serial_jtag=on imu=on watchdogs=on brownout=on psram=on
```

The Bluetooth memory release is intentionally one-way for the current boot. Code that later needs BLE must change the product power policy rather than silently starting the radio.

## What this does not claim

The current ESPsand firmware did not explicitly start Wi-Fi or Bluetooth before this policy existed. Therefore this change primarily establishes and enforces a no-radio invariant; it should not be presented as a measured temperature reduction until physical current/temperature evidence exists.

No CPU underclock is applied yet. CPU frequency should be selected from measured simulation/render headroom rather than guessed. Light sleep is also deferred because the current development path depends on stable native USB, deterministic timing, IMU polling and LED updates.

The LED matrix remains the dominant thermal/current concern at high luminance. The existing conservative development brightness ceiling stays in force, and a centralized current/brightness budget remains required before showcase-level full-panel brightness is used for sustained periods.
