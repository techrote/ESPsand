#pragma once

#include <cstdint>

#include <espsand/input/axis_transform.hpp>

namespace espsand::board {

enum class EvidenceStatus : std::uint8_t {
  kKnown,
  kAssumed,
  kNeedsPhysicalValidation,
};

struct PinFact {
  const char* signal;
  int gpio;
  EvidenceStatus status;
};

inline constexpr const char* kExpectedProductFamily = "Waveshare ESP32-S3-Matrix (SKU 27119)";
inline constexpr EvidenceStatus kProductFamilyStatus = EvidenceStatus::kKnown;

inline constexpr PinFact kMatrixData{"matrix_data", 14, EvidenceStatus::kKnown};
inline constexpr PinFact kImuSda{"imu_sda", 11, EvidenceStatus::kKnown};
inline constexpr PinFact kImuScl{"imu_scl", 12, EvidenceStatus::kKnown};
inline constexpr PinFact kImuInt1{"imu_int1", 10, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuInt2{"imu_int2", 13, EvidenceStatus::kAssumed};
inline constexpr PinFact kBootButton{"boot_button", 0, EvidenceStatus::kKnown};

inline constexpr std::uint8_t kQmiPreferredAddress = 0x6B;
inline constexpr std::uint8_t kQmiAlternateAddress = 0x6A;
inline constexpr std::uint8_t kQmiExpectedWhoAmI = 0x05;

// This is deliberately conservative while the physical board has not completed a thermal/current
// soak. It is a development ceiling, not a certified safe electrical limit.
inline constexpr std::uint8_t kInitialBrightnessCeiling = 32;

// Physical ES-002 calibration established that the original identity projection was rotated
// 90 degrees counter-clockwise relative to the visible panel. With screen coordinates defined as
// +x right and +y down, rotate the raw IMU XY projection 90 degrees clockwise so a physical
// downward gravity vector appears downward on the matrix:
//
//   matrix_x = -imu_y
//   matrix_y = +imu_x
//
// This locks the observed in-plane orientation. A later six-pose calibration will still map the
// full raw sensor frame into the human-facing USB/SIDE/FACE axes and confirm Z/sign conventions.
inline constexpr input::PlaneTransform kProvisionalMatrixTransform{
    {input::Axis::kY, -1},
    {input::Axis::kX, 1},
};

} // namespace espsand::board
