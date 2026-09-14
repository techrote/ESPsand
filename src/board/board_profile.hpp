#pragma once

#include <array>
#include <cstddef>
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
inline constexpr PinFact kImuInt1{"imu_int1", 10, EvidenceStatus::kKnown};
inline constexpr PinFact kImuInt2{"imu_int2", 13, EvidenceStatus::kKnown};
inline constexpr PinFact kBootButton{"boot_button", 0, EvidenceStatus::kKnown};
inline constexpr PinFact kUsbDm{"usb_dm", 19, EvidenceStatus::kKnown};
inline constexpr PinFact kUsbDp{"usb_dp", 20, EvidenceStatus::kKnown};

// The official Waveshare schematic shows GPIO1..GPIO7 routed directly to the exposed expansion
// header without another onboard load. All seven are native ESP32-S3 touch channels. GPIO8/9 are
// touch-capable in silicon but are not exposed on this board's header. GPIO10..GPIO14 are consumed
// by the QMI8658 and RGB matrix and are therefore intentionally excluded.
inline constexpr std::array<PinFact, 7> kTouchCandidates{{
    {"touch_candidate_1", 1, EvidenceStatus::kKnown},
    {"touch_candidate_2", 2, EvidenceStatus::kKnown},
    {"touch_candidate_3", 3, EvidenceStatus::kKnown},
    {"touch_candidate_4", 4, EvidenceStatus::kKnown},
    {"touch_candidate_5", 5, EvidenceStatus::kKnown},
    {"touch_candidate_6", 6, EvidenceStatus::kKnown},
    {"touch_candidate_7", 7, EvidenceStatus::kKnown},
}};

// Physical ES-003 characterization found that a fingertip spans most or all of the exposed
// GPIO1..7 edge, so pretending that the bare board provides reliable A/B buttons would overstate
// the hardware. The local groups remain diagnostic only: they are useful for visualizing a swipe,
// but they show occasional spurious amber activity and are not exposed to scenes.
inline constexpr std::array<std::size_t, 3> kProvisionalTouchZoneA{{4, 5, 6}}; // GPIO5..7
inline constexpr std::array<std::size_t, 3> kProvisionalTouchZoneB{{0, 1, 2}}; // GPIO1..3
inline constexpr bool kTouchLocalZonesConfigured = false;

// A deliberate edge pinch produces a strong, smooth common-mode response whose magnitude tracks
// broad fingertip/PCB contact area. ES-003 therefore exposes only the bounded combo/ambiguous
// capacitive channel as a supported semantic input on the bare board.
inline constexpr bool kTouchComboConfigured = true;
inline constexpr bool kTouchZonesConfigured = kTouchLocalZonesConfigured || kTouchComboConfigured;

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
