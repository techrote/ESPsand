#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>

namespace espsand::render {

inline constexpr std::uint32_t kMaximumFrameChannelSum =
    static_cast<std::uint32_t>(io::kMatrixPixels) * 3U * 255U;

// Dimensionless post-brightness aggregate PWM envelope. This is deliberately conservative and
// provisional; it is not a measured current or thermal safety rating for the physical board.
inline constexpr std::uint32_t kDefaultFrameLoadLimit = 4096U;

struct OutputPolicy {
  // Beauty rendering now owns the ordinary 0..127 / pseudo-HDR 128..255 luminance distinction.
  // The default scalar ceiling therefore permits the complete code range; aggregate load limiting
  // remains mandatory and may reduce the applied scalar for dense/high-energy frames.
  std::uint8_t brightness_ceiling = 255;
  std::uint32_t frame_load_limit = kDefaultFrameLoadLimit;
};

struct OutputDecision {
  std::uint8_t requested_brightness = 0;
  std::uint8_t applied_brightness = 0;
  std::uint32_t frame_channel_sum = 0;
  std::uint32_t estimated_frame_load = 0;
  bool ceiling_limited = false;
  bool load_limited = false;
};

class OutputLimiter {
public:
  explicit OutputLimiter(OutputPolicy policy = {}) noexcept;

  void set_policy(OutputPolicy policy) noexcept;
  const OutputPolicy& policy() const noexcept;

  OutputDecision limit(const io::Frame8x8& frame, std::uint8_t requested_brightness) const noexcept;

private:
  OutputPolicy policy_{};
};

} // namespace espsand::render
