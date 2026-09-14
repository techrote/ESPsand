#pragma once

#include <cstdint>

#include <espsand/io/touch_types.hpp>

namespace espsand::input {

struct TouchSemanticConfig {
  // The touch diagnostic renders common mode full-height at z=10. Requiring nearly that much
  // broad coupling plus at least two local threshold crossings keeps the slider deliberate.
  float slider_common_min_z = 9.5F;
  std::uint8_t slider_min_active_channels = 2;
  float slider_position_alpha = 0.45F;

  // A brief isolated near-full local bar is retained as an explicit external disturbance source.
  // It is not model PRNG state: deterministic simulation must receive it through InputFrame.
  float noise_min_z = 9.0F;
  float noise_common_max_z = 6.0F;
  std::uint16_t noise_cooldown_samples = 8;
};

struct TouchSemanticState {
  bool slider_active = false;
  float slider_position = 0.5F;
  float slider_strength = 0.0F;
  float noise_impulse = 0.0F;
  bool noise_event = false;
};

class TouchSemanticInterpreter {
public:
  explicit TouchSemanticInterpreter(TouchSemanticConfig config = {});

  void reset();
  TouchSemanticState update(const io::TouchDiagnostics& diagnostics);

private:
  TouchSemanticConfig config_{};
  float slider_position_ = 0.5F;
  bool slider_initialized_ = false;
  bool noise_candidate_active_ = false;
  std::uint16_t noise_cooldown_remaining_ = 0;
};

} // namespace espsand::input
