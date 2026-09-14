#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace espsand::io {

inline constexpr std::size_t kMaxTouchChannels = 7;

struct TouchChannelDiagnostics {
  std::uint32_t raw = 0;
  float baseline = 0.0F;
  float delta = 0.0F;
  float noise = 1.0F;
  float z_raw = 0.0F;
  float z = 0.0F;
  bool active = false;
};

struct TouchDiagnostics {
  std::array<TouchChannelDiagnostics, kMaxTouchChannels> channels{};
  std::uint8_t channel_count = 0;
  bool ready = false;
  float common_mode_z = 0.0F;
  float provisional_a = 0.0F;
  float provisional_b = 0.0F;
  float provisional_combo = 0.0F;
  bool provisional_a_active = false;
  bool provisional_b_active = false;
  bool provisional_combo_active = false;

  // Physically supported coarse direct-control interpretation. Slider position is meaningful only
  // while slider_active is true. The isolated noise signal is an explicit external input candidate,
  // never hidden model randomness.
  bool slider_active = false;
  float slider_position = 0.5F;
  float slider_strength = 0.0F;
  float noise_impulse = 0.0F;
  bool noise_event = false;
};

struct TouchFrame {
  std::uint64_t timestamp_us = 0;
  bool available = false;
  float cap_a = 0.0F;
  float cap_b = 0.0F;
  float cap_combo = 0.0F;
  bool event_a = false;
  bool event_b = false;
  bool event_combo = false;

  bool slider_active = false;
  float slider_position = 0.5F;
  float slider_strength = 0.0F;
  float noise_impulse = 0.0F;
  bool noise_event = false;
};

struct TouchStatus {
  bool hardware_available = false;
  bool zones_configured = false;
  std::uint8_t channel_count = 0;
  std::uint32_t sample_count = 0;
  std::uint32_t failure_count = 0;
  std::uint64_t last_sample_us = 0;
};

} // namespace espsand::io
