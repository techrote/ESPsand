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
inline constexpr PinFact kImuSda{"imu_sda", 11, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuScl{"imu_scl", 12, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuInt1{"imu_int1", 10, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuInt2{"imu_int2", 13, EvidenceStatus::kAssumed};
inline constexpr PinFact kBootButton{"boot_button", 0, EvidenceStatus::kAssumed};

inline constexpr std::uint8_t kQmiPreferredAddress = 0x6B;
inline constexpr std::uint8_t kQmiAlternateAddress = 0x6A;
inline constexpr std::uint8_t kQmiExpectedWhoAmI = 0x05;

// This is deliberately conservative while the physical board has not completed a thermal/current
// soak. It is a development ceiling, not a certified safe electrical limit.
inline constexpr std::uint8_t kInitialBrightnessCeiling = 32;

// Physical matrix-relative IMU orientation remains unverified. ES-002 exposes this transform
// explicitly so physical evidence can change it without touching scene/model logic.
inline constexpr input::PlaneTransform kProvisionalMatrixTransform{
    {input::Axis::kX, 1},
    {input::Axis::kY, 1},
};

} // namespace espsand::board
