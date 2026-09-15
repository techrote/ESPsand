#include <espsand/sim/prng.hpp>

namespace espsand::sim {
namespace {

constexpr std::uint64_t kPcgMultiplier = 6364136223846793005ULL;

} // namespace

Pcg32::Pcg32(std::uint64_t seed, std::uint64_t sequence) noexcept {
  reseed(seed, sequence);
}

void Pcg32::reseed(std::uint64_t seed, std::uint64_t sequence) noexcept {
  state_ = 0;
  increment_ = (sequence << 1U) | 1U;
  static_cast<void>(next_u32());
  state_ += seed;
  static_cast<void>(next_u32());
}

std::uint32_t Pcg32::next_u32() noexcept {
  const std::uint64_t old_state = state_;
  state_ = old_state * kPcgMultiplier + increment_;

  const auto xorshifted =
      static_cast<std::uint32_t>(((old_state >> 18U) ^ old_state) >> 27U);
  const auto rotation = static_cast<std::uint32_t>(old_state >> 59U);

  return (xorshifted >> rotation) |
         (xorshifted << ((0U - rotation) & 31U));
}

std::uint32_t Pcg32::bounded(std::uint32_t bound) noexcept {
  if (bound == 0) {
    return 0;
  }

  const std::uint32_t threshold = (0U - bound) % bound;
  for (;;) {
    const std::uint32_t value = next_u32();
    if (value >= threshold) {
      return value % bound;
    }
  }
}

} // namespace espsand::sim
