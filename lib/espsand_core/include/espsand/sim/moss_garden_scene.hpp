#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/prng.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kMossGardenSceneSchemaVersion = 2;
inline constexpr std::size_t kMaxMites = 3;
inline constexpr std::uint8_t kMossShootFlag = 0x01U;

struct MiteState {
  std::uint8_t x = 0;
  std::uint8_t y = 0;
  std::uint8_t energy = 0;
  bool active = false;
};

struct MossGardenStateSnapshot {
  std::array<MiteState, kMaxMites> mites{};
  std::uint8_t mite_count = 0;
  std::uint16_t growth_energy = 0;
};

struct MossGardenSceneStats {
  std::uint16_t growth_cells = 0;
  std::uint16_t reinforced_cells = 0;
  std::uint16_t mite_moves = 0;
  std::uint16_t feeds = 0;
  std::uint16_t starved = 0;
  std::uint16_t rain_pulses = 0;
  std::uint16_t seed_pulses = 0;
  std::uint16_t scatter_events = 0;
};

struct MossGardenTickContext {
  World& world;
  Pcg32& prng;
  const InputFrame& input;
  std::uint64_t tick;
  WorkBudget& event_budget;
};

class MossGardenScene {
public:
  void initialize(World& world, Pcg32& prng) noexcept;
  MossGardenSceneStats before_dynamics(MossGardenTickContext context) noexcept;
  void after_dynamics(MossGardenTickContext context, MossGardenSceneStats& stats) noexcept;

  MossGardenStateSnapshot snapshot() const noexcept;
  bool invariants_hold(const World& world) const noexcept;

private:
  std::array<MiteState, kMaxMites> mites_{};
  std::uint8_t mite_count_ = 0;
  std::uint16_t growth_energy_ = 0;
};

} // namespace espsand::sim
