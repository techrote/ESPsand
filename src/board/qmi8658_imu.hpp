#pragma once

#include <cstddef>
#include <cstdint>

#include <Wire.h>
#include <espsand/io/interfaces.hpp>

namespace espsand::board {

class Qmi8658Imu final : public io::IImu {
public:
  explicit Qmi8658Imu(TwoWire& wire = Wire);

  bool begin();
  bool poll(io::ImuSample& sample) override;
  io::ImuStatus status() const override;

private:
  bool probe(std::uint8_t address);
  bool configure();
  bool read_register(std::uint8_t reg, std::uint8_t& value);
  bool read_block(std::uint8_t reg, std::uint8_t* data, std::size_t length);
  bool write_register(std::uint8_t reg, std::uint8_t value);
  static std::int16_t decode_le_i16(const std::uint8_t* data);

  TwoWire& wire_;
  io::ImuStatus status_{};
  std::uint32_t consecutive_failures_ = 0;
};

} // namespace espsand::board
