#pragma once

#include <cstdint>

namespace espsand::sim {

class Pcg32 {
public:
  static constexpr std::uint64_t kDefaultSequence = 54U;

  explicit Pcg32(std::uint64_t seed = 0, std::uint64_t sequence = kDefaultSequence) noexcept;

  void reseed(std::uint64_t seed, std::uint64_t sequence = kDefaultSequence) noexcept;
  std::uint32_t next_u32() noexcept;
  std::uint32_t bounded(std::uint32_t bound) noexcept;

  std::uint64_t state() const noexcept {
    return state_;
  }

  std::uint64_t increment() const noexcept {
    return increment_;
  }

private:
  std::uint64_t state_ = 0;
  std::uint64_t increment_ = 1;
};

} // namespace espsand::sim
