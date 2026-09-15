#include "matrix_output.hpp"

#include <algorithm>
#include <cstddef>

#include "board_profile.hpp"

namespace espsand::board {

// Physical ES-002 colour calibration on the owner's Waveshare ESP32-S3-Matrix showed that
// logical red/green were swapped when this adapter used NEO_GRB: the requested
// red -> green -> blue diagnostic appeared green -> red -> blue, and amber appeared lime.
// The onboard chain therefore uses RGB byte order for ESPsand's logical Rgb contract.
MatrixOutput::MatrixOutput()
    : strip_(io::kMatrixPixels, kMatrixData.gpio, NEO_RGB + NEO_KHZ800),
      limiter_(render::OutputPolicy{kInitialBrightnessCeiling, render::kDefaultFrameLoadLimit}) {}

void MatrixOutput::begin() {
  strip_.begin();
  strip_.clear();
  strip_.setBrightness(0);
  strip_.show();
}

void MatrixOutput::present(const io::Frame8x8& frame, std::uint8_t requested_brightness) {
  last_output_decision_ = limiter_.limit(frame, requested_brightness);
  strip_.setBrightness(last_output_decision_.applied_brightness);
  for (std::size_t index = 0; index < frame.size(); ++index) {
    const auto& pixel = frame[index];
    strip_.setPixelColor(index, strip_.Color(pixel.r, pixel.g, pixel.b));
  }
  strip_.show();
}

std::uint8_t MatrixOutput::brightness_ceiling() const {
  return limiter_.policy().brightness_ceiling;
}

void MatrixOutput::set_output_policy(render::OutputPolicy policy) {
  policy.brightness_ceiling = std::min(policy.brightness_ceiling, kInitialBrightnessCeiling);
  limiter_.set_policy(policy);
}

std::uint32_t MatrixOutput::frame_load_limit() const {
  return limiter_.policy().frame_load_limit;
}

render::OutputDecision MatrixOutput::last_output_decision() const {
  return last_output_decision_;
}

} // namespace espsand::board
