#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/input/touch_normalizer.hpp>
#include <espsand/input/touch_semantics.hpp>
#include <espsand/io/interfaces.hpp>

namespace espsand::board {

class TouchZones final : public io::ITouchZones {
public:
  bool begin();
  bool poll(std::uint64_t now_us, io::TouchFrame& frame) override;
  io::TouchStatus status() const override;
  io::TouchDiagnostics diagnostics() const override;

private:
  static constexpr std::uint64_t kChannelPeriodUs = 4000;

  float group_max(const std::array<std::size_t, 3>& indices) const;
  void update_semantics(io::TouchFrame& frame, std::uint64_t now_us);

  input::TouchNormalizer normalizer_{};
  input::TouchSemanticInterpreter semantic_interpreter_{};
  input::TouchZoneGate zone_a_gate_{};
  input::TouchZoneGate zone_b_gate_{};
  input::TouchZoneGate combo_gate_{};
  std::array<std::uint32_t, io::kMaxTouchChannels> raw_{};
  io::TouchDiagnostics diagnostics_{};
  io::TouchStatus status_{};
  std::size_t next_channel_ = 0;
  std::uint64_t next_channel_us_ = 0;
};

} // namespace espsand::board
