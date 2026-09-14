#include "touch_zones.hpp"

#include <Arduino.h>

#include <algorithm>

#include "board_profile.hpp"

namespace espsand::board {

bool TouchZones::begin() {
  status_ = {};
  diagnostics_ = {};
  raw_ = {};
  next_channel_ = 0;
  next_channel_us_ = 0;
  zone_a_gate_.reset();
  zone_b_gate_.reset();
  combo_gate_.reset();

  for (const auto& candidate : kTouchCandidates) {
    if (digitalPinToTouchChannel(static_cast<std::uint8_t>(candidate.gpio)) < 0) {
      ++status_.failure_count;
    }
  }

  if (status_.failure_count != 0) {
    return false;
  }

  // touchRead() lazily configures each channel and Arduino-ESP32 2.0.17 deliberately waits
  // after first configuration. Pay that one-time cost during startup rather than injecting seven
  // long first-use stalls into the main loop.
  for (std::size_t index = 0; index < kTouchCandidates.size(); ++index) {
    raw_[index] = touchRead(static_cast<std::uint8_t>(kTouchCandidates[index].gpio));
  }

  normalizer_.reset(kTouchCandidates.size());
  status_.hardware_available = true;
  status_.zones_configured = kTouchZonesConfigured;
  status_.channel_count = static_cast<std::uint8_t>(kTouchCandidates.size());
  return true;
}

bool TouchZones::poll(std::uint64_t now_us, io::TouchFrame& frame) {
  frame = {};
  frame.timestamp_us = now_us;
  if (!status_.hardware_available) {
    return false;
  }

  if (next_channel_us_ != 0 && now_us < next_channel_us_) {
    return false;
  }
  next_channel_us_ = now_us + kChannelPeriodUs;

  raw_[next_channel_] = touchRead(static_cast<std::uint8_t>(kTouchCandidates[next_channel_].gpio));
  ++next_channel_;
  if (next_channel_ < kTouchCandidates.size()) {
    return false;
  }

  next_channel_ = 0;
  diagnostics_ = normalizer_.update(raw_, kTouchCandidates.size());
  ++status_.sample_count;
  status_.last_sample_us = now_us;
  update_provisional_groups(frame, now_us);
  return true;
}

io::TouchStatus TouchZones::status() const {
  return status_;
}

io::TouchDiagnostics TouchZones::diagnostics() const {
  return diagnostics_;
}

float TouchZones::group_max(const std::array<std::size_t, 3>& indices) const {
  float value = 0.0F;
  for (const std::size_t index : indices) {
    value = std::max(value, diagnostics_.channels[index].z);
  }
  return value;
}

void TouchZones::update_provisional_groups(io::TouchFrame& frame, std::uint64_t now_us) {
  frame.timestamp_us = now_us;
  if (!diagnostics_.ready) {
    zone_a_gate_.reset();
    zone_b_gate_.reset();
    combo_gate_.reset();
    return;
  }

  const float a_z = group_max(kProvisionalTouchZoneA);
  const float b_z = group_max(kProvisionalTouchZoneB);
  const float common_z = std::max(0.0F, diagnostics_.common_mode_z);

  const auto a = zone_a_gate_.update(a_z);
  const auto b = zone_b_gate_.update(b_z);
  // Physical ES-003 evidence found occasional local-channel false activity but a strong, smooth
  // edge-pinch response in the common-mode channel. Keep combo semantics strictly common-mode so
  // local amber flashes cannot become product events.
  const auto combo = combo_gate_.update(common_z);

  diagnostics_.provisional_a = a.intensity;
  diagnostics_.provisional_b = b.intensity;
  diagnostics_.provisional_combo = combo.intensity;
  diagnostics_.provisional_a_active = a.active;
  diagnostics_.provisional_b_active = b.active;
  diagnostics_.provisional_combo_active = combo.active;

  frame.available = status_.zones_configured;
  if (!frame.available) {
    return;
  }

  if (kTouchLocalZonesConfigured) {
    frame.cap_a = a.intensity;
    frame.cap_b = b.intensity;
    frame.event_a = a.triggered;
    frame.event_b = b.triggered;
  }

  if (kTouchComboConfigured) {
    frame.cap_combo = combo.intensity;
    frame.event_combo = combo.triggered;
  }
}

} // namespace espsand::board
