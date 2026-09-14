#pragma once

#include <cstdint>

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

inline constexpr const char* kExpectedProductFamily = "Waveshare ESP32-S3-Matrix / compatible";
inline constexpr EvidenceStatus kProductFamilyStatus = EvidenceStatus::kAssumed;

inline constexpr PinFact kMatrixData{"matrix_data", 14, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuSda{"imu_sda", 11, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuScl{"imu_scl", 12, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuInt1{"imu_int1", 10, EvidenceStatus::kAssumed};
inline constexpr PinFact kImuInt2{"imu_int2", 13, EvidenceStatus::kAssumed};
inline constexpr PinFact kBootButton{"boot_button", 0, EvidenceStatus::kAssumed};

}  // namespace espsand::board
