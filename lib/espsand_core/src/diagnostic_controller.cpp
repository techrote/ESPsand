#include <espsand/runtime/diagnostic_controller.hpp>

namespace espsand::runtime {

void DiagnosticController::initialize(std::uint32_t now_ms) {
  mode_started_ms_ = now_ms;
}

void DiagnosticController::handle_button(io::ButtonEvent event, std::uint32_t now_ms) {
  if (event == io::ButtonEvent::kShortPress) {
    mode_started_ms_ = now_ms;
    return;
  }

  if (event == io::ButtonEvent::kLongPress) {
    const auto next =
        (static_cast<std::uint8_t>(mode_) + 1U) % static_cast<std::uint8_t>(DiagnosticMode::kCount);
    mode_ = static_cast<DiagnosticMode>(next);
    mode_started_ms_ = now_ms;
  }
}

DiagnosticMode DiagnosticController::mode() const {
  return mode_;
}

std::uint32_t DiagnosticController::elapsed_ms(std::uint32_t now_ms) const {
  return now_ms - mode_started_ms_;
}

const char* diagnostic_mode_name(DiagnosticMode mode) {
  switch (mode) {
  case DiagnosticMode::kPixelSweep:
    return "pixel_sweep";
  case DiagnosticMode::kPrimaryColours:
    return "primary_colours";
  case DiagnosticMode::kGravity:
    return "gravity";
  case DiagnosticMode::kCount:
    break;
  }
  return "invalid";
}

} // namespace espsand::runtime
