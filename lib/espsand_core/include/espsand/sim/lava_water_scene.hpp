#pragma once

#include <cstdint>

#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/prng.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kLavaWaterSceneSchemaVersion = 1;

struct LavaWaterSceneStats {
  std::uint16_t autonomous_lava_injections = 0;
  std::uint16_t autonomous_water_injections = 0;
  std::uint16_t touch_lava_injections = 0;
  std::uint16_t burst_pairs = 0;
  std::uint16_t crust_fractures = 0;
  std::uint8_t last_injection_x = 0;
};

struct LavaWaterTickContext {
  World& world;
  Pcg32& prng;
  const InputFrame& input;
  std::uint64_t tick;
  WorkBudget& event_budget;
};

class LavaWaterScene {
public:
  void initialize(World& world, Pcg32& prng) const noexcept;
  LavaWaterSceneStats before_dynamics(LavaWaterTickContext context) const noexcept;
};

} // namespace espsand::sim
