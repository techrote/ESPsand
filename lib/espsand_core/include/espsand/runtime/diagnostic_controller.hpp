#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>

namespace espsand::runtime {

enum class DiagnosticMode : std::uint8_t {
  kPixelSweep,
  kPrimaryColours,
  kGravity,
  kTouchCharacterization,
  kCount,
};

class DiagnosticController {
public:
  void initialize(std::uint32_t now_ms);
  void handle_button(io::ButtonEvent event, std::uint32_t now_ms);

  DiagnosticMode mode() const;
  std::uint32_t elapsed_ms(std::uint32_t now_ms) const;

private:
  DiagnosticMode mode_ = DiagnosticMode::kPixelSweep;
  std::uint32_t mode_started_ms_ = 0;
};

const char* diagnostic_mode_name(DiagnosticMode mode);

} // namespace espsand::runtime
