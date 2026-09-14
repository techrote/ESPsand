#pragma once

#include <cstdint>

#include <espsand/input/button_gesture.hpp>
#include <espsand/io/interfaces.hpp>

namespace espsand::board {

class BootButton final : public io::IButton {
public:
  void begin();
  io::ButtonEvent poll(std::uint64_t now_us) override;

private:
  bool read_pressed() const;

  input::ButtonGesture gesture_{};
  bool initialized_ = false;
};

} // namespace espsand::board
