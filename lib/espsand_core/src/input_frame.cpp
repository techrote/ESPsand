#include <espsand/sim/input_frame.hpp>

#include <cmath>

namespace espsand::sim {
namespace {

float clamp_finite(float value, float minimum, float maximum, float fallback) noexcept {
  if (!std::isfinite(value)) {
    return fallback;
  }
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

BootEvent sanitize_boot_event(BootEvent event) noexcept {
  switch (event) {
  case BootEvent::kNone:
  case BootEvent::kResetScene:
  case BootEvent::kNextScene:
    return event;
  }

  return BootEvent::kNone;
}

} // namespace

InputFrame sanitize_input_frame(const InputFrame& input) noexcept {
  InputFrame output = input;

  output.gravity.x = clamp_finite(input.gravity.x, -1.0F, 1.0F, 0.0F);
  output.gravity.y = clamp_finite(input.gravity.y, -1.0F, 1.0F, 0.0F);
  output.gravity_magnitude = clamp_finite(input.gravity_magnitude, 0.0F, 1.0F, 0.0F);
  output.gravity_confidence = clamp_finite(input.gravity_confidence, 0.0F, 1.0F, 0.0F);

  output.shake_energy = clamp_finite(input.shake_energy, 0.0F, 1.0F, 0.0F);
  output.motion_energy = clamp_finite(input.motion_energy, 0.0F, 1.0F, 0.0F);
  output.tap_impulse = clamp_finite(input.tap_impulse, 0.0F, 1.0F, 0.0F);
  output.spin_rate = clamp_finite(input.spin_rate, -1.0F, 1.0F, 0.0F);

  output.boot_event = sanitize_boot_event(input.boot_event);

  output.cap_combo = clamp_finite(input.cap_combo, 0.0F, 1.0F, 0.0F);
  output.slider_position = clamp_finite(input.slider_position, 0.0F, 1.0F, 0.5F);
  output.slider_strength = clamp_finite(input.slider_strength, 0.0F, 1.0F, 0.0F);
  output.noise_impulse = clamp_finite(input.noise_impulse, 0.0F, 1.0F, 0.0F);

  return output;
}

} // namespace espsand::sim
