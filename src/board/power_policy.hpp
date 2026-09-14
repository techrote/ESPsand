#pragma once

namespace espsand::board {

struct PowerPolicyStatus {
  bool applied = false;
  bool wifi_off = false;
  bool bluetooth_off = false;
  bool bluetooth_memory_released = false;
  int wifi_error = 0;
  int bluetooth_error = 0;
};

// ESPsand v0 does not use wireless networking. Apply the board-level thermal/power
// policy before starting the rest of the runtime so an accidentally initialized
// radio is stopped and Bluetooth controller memory is released.
void apply_power_policy();

const PowerPolicyStatus& power_policy_status();

} // namespace espsand::board
