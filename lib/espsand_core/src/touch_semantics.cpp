#include <espsand/input/touch_semantics.hpp>

#include <algorithm>

namespace espsand::input {
namespace {

float clamp01(float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

} // namespace

TouchSemanticInterpreter::TouchSemanticInterpreter(TouchSemanticConfig config) : config_(config) {}

void TouchSemanticInterpreter::reset() {
  slider_position_ = 0.5F;
  slider_initialized_ = false;
  noise_candidate_active_ = false;
  noise_cooldown_remaining_ = 0;
}

TouchSemanticState TouchSemanticInterpreter::update(const io::TouchDiagnostics& diagnostics) {
  TouchSemanticState result{};
  result.slider_position = slider_position_;

  if (!diagnostics.ready || diagnostics.channel_count == 0) {
    noise_candidate_active_ = false;
    return result;
  }

  if (noise_cooldown_remaining_ > 0) {
    --noise_cooldown_remaining_;
  }

  std::uint8_t active_count = 0;
  float weight_sum = 0.0F;
  float position_sum = 0.0F;
  float max_local_z = 0.0F;

  const std::size_t count = std::min<std::size_t>(diagnostics.channel_count, io::kMaxTouchChannels);
  for (std::size_t index = 0; index < count; ++index) {
    const auto& channel = diagnostics.channels[index];
    if (channel.active) {
      ++active_count;
    }

    const float local_z = std::max(channel.z, 0.0F);
    max_local_z = std::max(max_local_z, local_z);
    if (local_z <= 0.0F) {
      continue;
    }

    float position = 0.5F;
    if (count > 1) {
      position = static_cast<float>(index) / static_cast<float>(count - 1);
    }
    weight_sum += local_z;
    position_sum += local_z * position;
  }

  const float common_z = std::max(diagnostics.common_mode_z, 0.0F);
  const bool slider_gate = active_count >= config_.slider_min_active_channels &&
                           common_z >= config_.slider_common_min_z && weight_sum > 0.0F;

  if (slider_gate) {
    const float target = clamp01(position_sum / weight_sum);
    if (!slider_initialized_) {
      slider_position_ = target;
      slider_initialized_ = true;
    } else {
      slider_position_ += config_.slider_position_alpha * (target - slider_position_);
    }
    result.slider_active = true;
    result.slider_position = clamp01(slider_position_);
    result.slider_strength = clamp01(common_z / 10.0F);
  }

  const bool isolated_noise = active_count == 1 && max_local_z >= config_.noise_min_z &&
                              common_z <= config_.noise_common_max_z;
  result.noise_impulse = isolated_noise ? clamp01(max_local_z / 10.0F) : 0.0F;
  if (isolated_noise && !noise_candidate_active_ && noise_cooldown_remaining_ == 0) {
    result.noise_event = true;
    noise_cooldown_remaining_ = config_.noise_cooldown_samples;
  }
  noise_candidate_active_ = isolated_noise;

  return result;
}

} // namespace espsand::input
