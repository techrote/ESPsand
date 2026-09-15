#pragma once

#include <cstdint>

#include <Adafruit_NeoPixel.h>
#include <espsand/io/interfaces.hpp>
#include <espsand/render/output_limiter.hpp>

namespace espsand::board {

class MatrixOutput final : public io::IMatrixOutput {
public:
  MatrixOutput();

  void begin();
  void present(const io::Frame8x8& frame, std::uint8_t requested_brightness) override;
  std::uint8_t brightness_ceiling() const override;

  void set_output_policy(render::OutputPolicy policy);
  std::uint32_t frame_load_limit() const;
  render::OutputDecision last_output_decision() const;

private:
  Adafruit_NeoPixel strip_;
  render::OutputLimiter limiter_;
  render::OutputDecision last_output_decision_{};
};

} // namespace espsand::board
