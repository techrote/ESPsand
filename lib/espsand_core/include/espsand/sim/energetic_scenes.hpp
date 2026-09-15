#pragma once

#include <cstdint>

#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/prng.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kSodiumWaterSceneSchemaVersion = 2;
inline constexpr std::uint32_t kOilFireSceneSchemaVersion = 2;

struct EnergeticSceneTickContext {
  World& world;
  Pcg32& prng;
  const InputFrame& input;
  std::uint64_t tick;
  WorkBudget& event_budget;
};

struct SodiumWaterSceneStats {
  std::uint16_t autonomous_sodium_injections = 0;
  std::uint16_t autonomous_water_injections = 0;
  std::uint16_t touch_sodium_injections = 0;
  std::uint16_t burst_pairs = 0;
  std::uint8_t last_injection_x = 0;
};

struct OilFireSceneStats {
  std::uint16_t autonomous_oil_injections = 0;
  std::uint16_t autonomous_ignitions = 0;
  std::uint16_t touch_oil_injections = 0;
  std::uint16_t combo_ignitions = 0;
  std::uint8_t last_injection_x = 0;
};

class SodiumWaterScene {
public:
  void initialize(World& world, Pcg32& prng) const noexcept;
  SodiumWaterSceneStats before_dynamics(EnergeticSceneTickContext context) const noexcept;
};

class OilFireScene {
public:
  void initialize(World& world, Pcg32& prng) const noexcept;
  OilFireSceneStats before_dynamics(EnergeticSceneTickContext context) const noexcept;
};

} // namespace espsand::sim
