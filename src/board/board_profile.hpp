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

// A deliberate edge pinch produces a strong common-mode response. That remains the broad
// combo/ambiguous capacitive semantic. A second, intentionally coarse direct-control interpretation
// is allowed only when common mode is near diagnostic full scale and at least two local channels
// are simultaneously active; their weighted centroid becomes a 0..1 slider position.
inline constexpr bool kTouchComboConfigured = true;
inline constexpr bool kTouchSliderConfigured = true;

// Fast isolated single-channel full-height-ish excursions are preserved as an optional external
// disturbance/noise impulse. This must enter deterministic simulation explicitly through
// InputFrame; it is never folded into hidden model PRNG state.
inline constexpr bool kTouchNoiseConfigured = true;
inline constexpr bool kTouchZonesConfigured = kTouchLocalZonesConfigured || kTouchComboConfigured ||
                                              kTouchSliderConfigured || kTouchNoiseConfigured;

inline constexpr std::uint8_t kQmiPreferredAddress = 0x6B;
inline constexpr std::uint8_t kQmiAlternateAddress = 0x6A;
inline constexpr std::uint8_t kQmiExpectedWhoAmI = 0x05;

// Beauty rendering now reserves 0..127 for ordinary material and 128..255 for sparse pseudo-HDR
// energetic/highlight state. Allowing the full scalar code range here is not a declaration that a
// dense 255 frame is safe: every frame remains subject to the centralized 4096-unit aggregate load
// limiter, and electrical/thermal limits still require physical validation.
inline constexpr std::uint8_t kInitialBrightnessCeiling = 255;

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
