#include <espsand/render/output_limiter.hpp>

#include <algorithm>
#include <cstdint>

namespace espsand::render {
namespace {

std::uint32_t frame_channel_sum(const io::Frame8x8& frame) noexcept {
  std::uint32_t sum = 0;
  for (const io::Rgb& pixel : frame) {
    sum += pixel.r;
    sum += pixel.g;
    sum += pixel.b;
  }
  return sum;
}

std::uint32_t estimate_load(std::uint32_t channel_sum, std::uint8_t brightness) noexcept {
  return (channel_sum * static_cast<std::uint32_t>(brightness) + 127U) / 255U;
}

} // namespace

OutputLimiter::OutputLimiter(OutputPolicy policy) noexcept : policy_(policy) {}

void OutputLimiter::set_policy(OutputPolicy policy) noexcept {
  policy_ = policy;
}

const OutputPolicy& OutputLimiter::policy() const noexcept {
  return policy_;
}

OutputDecision OutputLimiter::limit(const io::Frame8x8& frame,
                                    std::uint8_t requested_brightness) const noexcept {
  OutputDecision decision{};
  decision.requested_brightness = requested_brightness;
  decision.frame_channel_sum = frame_channel_sum(frame);

  std::uint8_t applied = std::min(requested_brightness, policy_.brightness_ceiling);
  decision.ceiling_limited = applied != requested_brightness;

  if (decision.frame_channel_sum != 0U && policy_.frame_load_limit != 0U) {
    const std::uint32_t current_load = estimate_load(decision.frame_channel_sum, applied);
    if (current_load > policy_.frame_load_limit) {
      const std::uint32_t allowed =
          (policy_.frame_load_limit * 255U) / decision.frame_channel_sum;
      applied = static_cast<std::uint8_t>(std::min<std::uint32_t>(applied, allowed));
      decision.load_limited = true;
    }
  } else if (policy_.frame_load_limit == 0U && decision.frame_channel_sum != 0U) {
    applied = 0;
    decision.load_limited = true;
  }

  decision.applied_brightness = applied;
  decision.estimated_frame_load = estimate_load(decision.frame_channel_sum, applied);
  return decision;
}

} // namespace espsand::render
