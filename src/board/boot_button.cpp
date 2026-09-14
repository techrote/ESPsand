#include "boot_button.hpp"

#include <Arduino.h>

#include "board_profile.hpp"

namespace espsand::board {

void BootButton::begin() {
  pinMode(kBootButton.gpio, INPUT_PULLUP);
  gesture_.reset(read_pressed(), millis());
  initialized_ = true;
}

io::ButtonEvent BootButton::poll(std::uint64_t now_us) {
  if (!initialized_) {
    begin();
  }
  const auto now_ms = static_cast<std::uint32_t>(now_us / 1000U);
  return gesture_.update(read_pressed(), now_ms);
}

bool BootButton::read_pressed() const { return digitalRead(kBootButton.gpio) == LOW; }

} // namespace espsand::board
