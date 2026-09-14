#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/io/touch_types.hpp>

namespace espsand::input {

struct TouchNormalizerConfig {
  float baseline_alpha_idle = 0.015F;
  float baseline_alpha_active = 0.001F;
  float baseline_alpha_warmup = 0.15F;
  float noise_alpha_idle = 0.05F;
  float noise_alpha_active = 0.002F;
  float noise_floor = 4.0F;
  float baseline_hold_z = 3.0F;
  float common_mode_gain = 1.0F;
  float channel_enter_z = 5.0F;
  float channel_exit_z = 2.5F;
  std::uint16_t warmup_samples = 32;
};

class TouchNormalizer {
public:
  explicit TouchNormalizer(TouchNormalizerConfig config = {});

  void reset(std::size_t channel_count);
  io::TouchDiagnostics update(const std::array<std::uint32_t, io::kMaxTouchChannels>& raw,
                              std::size_t channel_count);

private:
  struct ChannelState {
    float baseline = 0.0F;
    float noise = 1.0F;
    bool initialized = false;
    bool active = false;
  };

  TouchNormalizerConfig config_{};
  std::array<ChannelState, io::kMaxTouchChannels> channels_{};
  std::size_t channel_count_ = 0;
  std::uint32_t sample_count_ = 0;
};

struct TouchGateConfig {
  float enter_z = 5.0F;
  float exit_z = 2.5F;
  float full_scale_z = 12.0F;
  std::uint16_t cooldown_samples = 8;
};

struct TouchGateState {
  float intensity = 0.0F;
  bool active = false;
  bool triggered = false;
};

class TouchZoneGate {
public:
  explicit TouchZoneGate(TouchGateConfig config = {});

  void reset();
  TouchGateState update(float normalized_disturbance);

private:
  TouchGateConfig config_{};
  bool active_ = false;
  std::uint16_t cooldown_remaining_ = 0;
};

} // namespace espsand::input
