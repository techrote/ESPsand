#pragma once

#include <cstdint>

#include <Adafruit_NeoPixel.h>
#include <espsand/io/interfaces.hpp>

namespace espsand::board {

class MatrixOutput final : public io::IMatrixOutput {
public:
  MatrixOutput();

  void begin();
  void present(const io::Frame8x8& frame, std::uint8_t requested_brightness) override;
  std::uint8_t brightness_ceiling() const override;

private:
  Adafruit_NeoPixel strip_;
};

} // namespace espsand::board
