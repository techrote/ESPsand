#include "qmi8658_imu.hpp"

#include <array>

#include <esp_timer.h>

#include "board_profile.hpp"

namespace espsand::board {
namespace {

constexpr std::uint8_t kRegWhoAmI = 0x00;
constexpr std::uint8_t kRegRevision = 0x01;
constexpr std::uint8_t kRegCtrl1 = 0x02;
constexpr std::uint8_t kRegCtrl2 = 0x03;
constexpr std::uint8_t kRegCtrl3 = 0x04;
constexpr std::uint8_t kRegCtrl4 = 0x05;
constexpr std::uint8_t kRegCtrl5 = 0x06;
constexpr std::uint8_t kRegCtrl6 = 0x07;
constexpr std::uint8_t kRegCtrl7 = 0x08;
constexpr std::uint8_t kRegAccelX = 0x35;
constexpr std::size_t kRawPayloadBytes = 12;
constexpr float kAccelCountsPerG = 4096.0F;
constexpr float kGyroCountsPerDps = 64.0F;
constexpr std::uint32_t kUnhealthyAfterFailures = 5;

} // namespace

Qmi8658Imu::Qmi8658Imu(TwoWire& wire) : wire_(wire) {}

bool Qmi8658Imu::begin() {
  status_ = {};
  consecutive_failures_ = 0;
  wire_.begin(kImuSda.gpio, kImuScl.gpio, 400000U);

  if (!probe(kQmiPreferredAddress) && !probe(kQmiAlternateAddress)) {
    return false;
  }

  if (!configure()) {
    return false;
  }

  status_.initialized = true;
  status_.healthy = true;
  return true;
}

bool Qmi8658Imu::poll(io::ImuSample& sample) {
  sample = {};
  if (!status_.initialized) {
    return false;
  }

  std::array<std::uint8_t, kRawPayloadBytes> raw{};
  if (!read_block(kRegAccelX, raw.data(), raw.size())) {
    ++status_.failure_count;
    ++consecutive_failures_;
    if (consecutive_failures_ >= kUnhealthyAfterFailures) {
      status_.healthy = false;
    }
    return false;
  }

  sample.timestamp_us = static_cast<std::uint64_t>(esp_timer_get_time());
  sample.accel_raw = {
      decode_le_i16(&raw[0]),
      decode_le_i16(&raw[2]),
      decode_le_i16(&raw[4]),
  };
  sample.gyro_raw = {
      decode_le_i16(&raw[6]),
      decode_le_i16(&raw[8]),
      decode_le_i16(&raw[10]),
  };
  sample.accel_g = {
      static_cast<float>(sample.accel_raw.x) / kAccelCountsPerG,
      static_cast<float>(sample.accel_raw.y) / kAccelCountsPerG,
      static_cast<float>(sample.accel_raw.z) / kAccelCountsPerG,
  };
  sample.gyro_dps = {
      static_cast<float>(sample.gyro_raw.x) / kGyroCountsPerDps,
      static_cast<float>(sample.gyro_raw.y) / kGyroCountsPerDps,
      static_cast<float>(sample.gyro_raw.z) / kGyroCountsPerDps,
  };
  sample.valid = true;

  consecutive_failures_ = 0;
  status_.healthy = true;
  ++status_.sample_count;
  status_.last_sample_us = sample.timestamp_us;
  return true;
}

io::ImuStatus Qmi8658Imu::status() const {
  return status_;
}

bool Qmi8658Imu::probe(std::uint8_t address) {
  status_.address = address;
  std::uint8_t who_am_i = 0;
  if (!read_register(kRegWhoAmI, who_am_i) || who_am_i != kQmiExpectedWhoAmI) {
    return false;
  }

  status_.who_am_i = who_am_i;
  std::uint8_t revision = 0;
  if (read_register(kRegRevision, revision)) {
    status_.revision = revision;
  }
  return true;
}

bool Qmi8658Imu::configure() {
  // Configuration follows the QMI8658 setup used by the Waveshare-family examples:
  // +/-8 g accelerometer, 1 kHz ODR, +/-512 dps gyro, 1 kHz ODR, LPFs enabled.
  return write_register(kRegCtrl1, 0x60) && write_register(kRegCtrl2, 0x23) &&
         write_register(kRegCtrl3, 0x53) && write_register(kRegCtrl4, 0x00) &&
         write_register(kRegCtrl5, 0x11) && write_register(kRegCtrl6, 0x00) &&
         write_register(kRegCtrl7, 0x03);
}

bool Qmi8658Imu::read_register(std::uint8_t reg, std::uint8_t& value) {
  return read_block(reg, &value, 1);
}

bool Qmi8658Imu::read_block(std::uint8_t reg, std::uint8_t* data, std::size_t length) {
  wire_.beginTransmission(status_.address);
  wire_.write(reg);
  if (wire_.endTransmission(false) != 0) {
    return false;
  }

  const auto requested = static_cast<std::uint8_t>(length);
  const auto received =
      wire_.requestFrom(status_.address, requested, static_cast<std::uint8_t>(true));
  if (received != requested) {
    while (wire_.available()) {
      wire_.read();
    }
    return false;
  }

  for (std::size_t index = 0; index < length; ++index) {
    const int value = wire_.read();
    if (value < 0) {
      return false;
    }
    data[index] = static_cast<std::uint8_t>(value);
  }
  return true;
}

bool Qmi8658Imu::write_register(std::uint8_t reg, std::uint8_t value) {
  wire_.beginTransmission(status_.address);
  wire_.write(reg);
  wire_.write(value);
  return wire_.endTransmission() == 0;
}

std::int16_t Qmi8658Imu::decode_le_i16(const std::uint8_t* data) {
  const auto value =
      static_cast<std::uint16_t>(data[0]) | (static_cast<std::uint16_t>(data[1]) << 8U);
  return static_cast<std::int16_t>(value);
}

} // namespace espsand::board
