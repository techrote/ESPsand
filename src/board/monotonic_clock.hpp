#pragma once

#include <cstdint>

#include <esp_timer.h>
#include <espsand/io/interfaces.hpp>

namespace espsand::board {

class MonotonicClock final : public io::IClock {
public:
  std::uint64_t now_us() const override {
    return static_cast<std::uint64_t>(esp_timer_get_time());
  }
};

} // namespace espsand::board
