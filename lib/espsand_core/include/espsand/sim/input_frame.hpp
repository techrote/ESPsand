#pragma once

#include <cstdint>

namespace espsand::sim {

struct NormalizedVec2 {
  float x = 0.0F;
  float y = 0.0F;
};

enum class BootEvent : std::uint8_t {
  kNone = 0,
  kResetScene,
  kNextScene,
};

struct InputFrame {
  NormalizedVec2 gravity{};
  float gravity_magnitude = 0.0F;
  float gravity_confidence = 0.0F;

  float shake_energy = 0.0F;
  float motion_energy = 0.0F;
  float tap_impulse = 0.0F;
  float spin_rate = 0.0F;

  BootEvent boot_event = BootEvent::kNone;

  float cap_combo = 0.0F;
  bool cap_combo_event = false;

  bool slider_active = false;
  float slider_position = 0.5F;
  float slider_strength = 0.0F;

  float noise_impulse = 0.0F;
  bool noise_event = false;
};

InputFrame sanitize_input_frame(const InputFrame& input) noexcept;

} // namespace espsand::sim
