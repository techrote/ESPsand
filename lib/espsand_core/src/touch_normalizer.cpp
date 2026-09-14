#include <espsand/input/touch_normalizer.hpp>

#include <algorithm>
#include <cmath>

namespace espsand::input {
namespace {

float clamp01(float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

} // namespace

TouchNormalizer::TouchNormalizer(TouchNormalizerConfig config) : config_(config) {}

void TouchNormalizer::reset(std::size_t channel_count) {
  channel_count_ = std::min(channel_count, io::kMaxTouchChannels);
  sample_count_ = 0;
  channels_ = {};
  for (auto& channel : channels_) {
    channel.noise = config_.noise_floor;
  }
}

io::TouchDiagnostics
TouchNormalizer::update(const std::array<std::uint32_t, io::kMaxTouchChannels>& raw,
                        std::size_t channel_count) {
  const std::size_t count = std::min(channel_count, io::kMaxTouchChannels);
  if (count != channel_count_) {
    reset(count);
  }

  io::TouchDiagnostics result{};
  result.channel_count = static_cast<std::uint8_t>(count);
  if (count == 0) {
    return result;
  }

  float common_mode_sum = 0.0F;
  for (std::size_t index = 0; index < count; ++index) {
    auto& state = channels_[index];
    auto& out = result.channels[index];
    out.raw = raw[index];

    if (!state.initialized) {
      state.baseline = static_cast<float>(raw[index]);
      state.noise = config_.noise_floor;
      state.initialized = true;
      out.baseline = state.baseline;
      out.noise = state.noise;
      continue;
    }

    out.baseline = state.baseline;
    out.delta = static_cast<float>(raw[index]) - state.baseline;
    out.noise = std::max(state.noise, config_.noise_floor);
    out.z_raw = out.delta / out.noise;
    common_mode_sum += out.z_raw;
  }

  result.common_mode_z = common_mode_sum / static_cast<float>(count);
  const bool warming_up = sample_count_ < config_.warmup_samples;

  for (std::size_t index = 0; index < count; ++index) {
    auto& state = channels_[index];
    auto& out = result.channels[index];

    out.z = out.z_raw - config_.common_mode_gain * result.common_mode_z;

    if (warming_up) {
      state.active = false;
    } else if (state.active) {
      state.active = out.z >= config_.channel_exit_z;
    } else {
      state.active = out.z >= config_.channel_enter_z;
    }
    out.active = state.active;

    const bool hold_baseline = state.active || std::fabs(out.z_raw) >= config_.baseline_hold_z;
    float baseline_alpha = config_.baseline_alpha_idle;
    if (warming_up) {
      baseline_alpha = config_.baseline_alpha_warmup;
    } else if (hold_baseline) {
      baseline_alpha = config_.baseline_alpha_active;
    }
    state.baseline += baseline_alpha * out.delta;

    const float residual_abs = std::fabs(out.delta);
    const float noise_alpha = hold_baseline ? config_.noise_alpha_active : config_.noise_alpha_idle;
    state.noise += noise_alpha * (residual_abs - state.noise);
    state.noise = std::max(state.noise, config_.noise_floor);
  }

  ++sample_count_;
  result.ready = sample_count_ >= config_.warmup_samples;
  return result;
}

TouchZoneGate::TouchZoneGate(TouchGateConfig config) : config_(config) {}

void TouchZoneGate::reset() {
  active_ = false;
  cooldown_remaining_ = 0;
}

TouchGateState TouchZoneGate::update(float normalized_disturbance) {
  if (cooldown_remaining_ > 0) {
    --cooldown_remaining_;
  }

  bool triggered = false;
  if (active_) {
    if (normalized_disturbance <= config_.exit_z) {
      active_ = false;
    }
  } else if (normalized_disturbance >= config_.enter_z) {
    active_ = true;
    if (cooldown_remaining_ == 0) {
      triggered = true;
      cooldown_remaining_ = config_.cooldown_samples;
    }
  }

  const float span = std::max(config_.full_scale_z - config_.exit_z, 0.001F);
  const float intensity = clamp01((normalized_disturbance - config_.exit_z) / span);
  return {intensity, active_, triggered};
}

} // namespace espsand::input
