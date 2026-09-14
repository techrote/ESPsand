#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>

namespace espsand::input {

struct ButtonGestureConfig {
  std::uint32_t debounce_ms = 30;
  std::uint32_t short_max_ms = 600;
  std::uint32_t long_min_ms = 800;
};

class ButtonGesture {
public:
  explicit ButtonGesture(ButtonGestureConfig config = {});

  void reset(bool initial_pressed, std::uint32_t now_ms);
  io::ButtonEvent update(bool raw_pressed, std::uint32_t now_ms);

  bool stable_pressed() const;

private:
  static std::uint32_t elapsed(std::uint32_t now, std::uint32_t then);

  ButtonGestureConfig config_{};
  bool initialized_ = false;
  bool raw_pressed_ = false;
  bool stable_pressed_ = false;
  bool long_emitted_ = false;
  std::uint32_t raw_changed_ms_ = 0;
  std::uint32_t pressed_since_ms_ = 0;
};

} // namespace espsand::input
