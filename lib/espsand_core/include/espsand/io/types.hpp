#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace espsand::io {

struct Vec2 {
  float x = 0.0F;
  float y = 0.0F;
};

struct Vec3 {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

struct RawVec3 {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t z = 0;
};

struct ImuSample {
  std::uint64_t timestamp_us = 0;
  RawVec3 accel_raw{};
  RawVec3 gyro_raw{};
  Vec3 accel_g{};
  Vec3 gyro_dps{};
  bool valid = false;
};

struct ImuStatus {
  bool initialized = false;
  bool healthy = false;
  std::uint8_t address = 0;
  std::uint8_t who_am_i = 0;
  std::uint8_t revision = 0;
  std::uint32_t sample_count = 0;
  std::uint32_t failure_count = 0;
  std::uint64_t last_sample_us = 0;
};

struct Rgb {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

inline constexpr std::size_t kMatrixWidth = 8;
inline constexpr std::size_t kMatrixHeight = 8;
inline constexpr std::size_t kMatrixPixels = kMatrixWidth * kMatrixHeight;
using Frame8x8 = std::array<Rgb, kMatrixPixels>;

enum class ButtonEvent : std::uint8_t {
  kNone,
  kShortPress,
  kLongPress,
};

} // namespace espsand::io
