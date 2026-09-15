#pragma once

#include <cstdint>
#include <limits>

namespace espsand::sim {

struct WorkBudgetStats {
  std::uint16_t limit = 0;
  std::uint16_t used = 0;
  std::uint16_t dropped = 0;
};

class WorkBudget {
public:
  explicit WorkBudget(std::uint16_t limit = 0) noexcept {
    reset(limit);
  }

  void reset(std::uint16_t limit) noexcept {
    stats_ = WorkBudgetStats{limit, 0, 0};
  }

  bool try_consume(std::uint16_t units = 1) noexcept {
    if (units == 0) {
      return true;
    }

    const std::uint16_t remaining = static_cast<std::uint16_t>(stats_.limit - stats_.used);
    if (units <= remaining) {
      stats_.used = static_cast<std::uint16_t>(stats_.used + units);
      return true;
    }

    const std::uint32_t dropped = static_cast<std::uint32_t>(stats_.dropped) + units;
    const auto maximum = std::numeric_limits<std::uint16_t>::max();
    stats_.dropped = dropped > maximum ? maximum : static_cast<std::uint16_t>(dropped);
    return false;
  }

  std::uint16_t remaining() const noexcept {
    return static_cast<std::uint16_t>(stats_.limit - stats_.used);
  }

  const WorkBudgetStats& stats() const noexcept {
    return stats_;
  }

private:
  WorkBudgetStats stats_{};
};

} // namespace espsand::sim
