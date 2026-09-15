#pragma once

#include <array>
#include <cstdint>

#include <espsand/sim/dynamics.hpp>
#include <espsand/sim/energetic_scenes.hpp>
#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/lava_water_scene.hpp>
#include <espsand/sim/moss_garden_scene.hpp>
#include <espsand/sim/prng.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kStateHashSchemaVersion = 1;

enum class SceneId : std::uint8_t {
  kDeterminismFixture = 0,
  kDynamicsFixture = 1,
  kLavaWater = 2,
  kSodiumWater = 3,
  kOilFire = 4,
  kMossGarden = 5,
};

inline constexpr std::array<SceneId, 4> kProductSceneOrder{{
    SceneId::kLavaWater,
    SceneId::kSodiumWater,
    SceneId::kOilFire,
    SceneId::kMossGarden,
}};

constexpr bool is_product_scene(SceneId scene) noexcept {
  return scene == SceneId::kLavaWater || scene == SceneId::kSodiumWater ||
         scene == SceneId::kOilFire || scene == SceneId::kMossGarden;
}

constexpr bool uses_shared_dynamics(SceneId scene) noexcept {
  return scene == SceneId::kDynamicsFixture || is_product_scene(scene);
}

constexpr const char* scene_name(SceneId scene) noexcept {
  switch (scene) {
  case SceneId::kDeterminismFixture:
    return "determinism_fixture";
  case SceneId::kDynamicsFixture:
    return "dynamics_fixture";
  case SceneId::kLavaWater:
    return "lava_water";
  case SceneId::kSodiumWater:
    return "sodium_water";
  case SceneId::kOilFire:
    return "oil_fire";
  case SceneId::kMossGarden:
    return "moss_garden";
  }
  return "invalid";
}

constexpr SceneId next_product_scene(SceneId scene) noexcept {
  switch (scene) {
  case SceneId::kLavaWater:
    return SceneId::kSodiumWater;
  case SceneId::kSodiumWater:
    return SceneId::kOilFire;
  case SceneId::kOilFire:
    return SceneId::kMossGarden;
  case SceneId::kMossGarden:
  case SceneId::kDeterminismFixture:
  case SceneId::kDynamicsFixture:
    return SceneId::kLavaWater;
  }
  return SceneId::kLavaWater;
}

struct ModelConfig {
  std::uint64_t seed = 1;
  SceneId scene = SceneId::kDeterminismFixture;
  std::uint16_t event_budget = 8;
  std::uint16_t reaction_budget = 8;
};

struct TickWorkStats {
  WorkBudgetStats events{};
  WorkBudgetStats reactions{};
};

struct FixtureStateSnapshot {
  std::uint8_t marker_x = 0;
  std::uint8_t marker_y = 0;
  std::uint8_t slider_position_q8 = 128;
  std::uint8_t slider_strength_q8 = 0;
  std::uint8_t cap_combo_q8 = 0;
  std::uint16_t external_impulse = 0;
};

class Model {
public:
  Model() noexcept;
  explicit Model(const ModelConfig& config) noexcept;

  void init(const ModelConfig& config) noexcept;
  void reset() noexcept;
  void reseed(std::uint64_t seed) noexcept;
  void step(const InputFrame& input) noexcept;

  const World& world() const noexcept {
    return world_;
  }

  const Pcg32& prng() const noexcept {
    return prng_;
  }

  std::uint64_t seed() const noexcept {
    return config_.seed;
  }

  std::uint64_t tick() const noexcept {
    return tick_;
  }

  SceneId scene() const noexcept {
    return config_.scene;
  }

  TickWorkStats tick_work_stats() const noexcept;
  FixtureStateSnapshot fixture_state() const noexcept;
  DynamicsStats dynamics_stats() const noexcept;
  LavaWaterSceneStats lava_water_stats() const noexcept;
  SodiumWaterSceneStats sodium_water_stats() const noexcept;
  OilFireSceneStats oil_fire_stats() const noexcept;
  MossGardenSceneStats moss_garden_stats() const noexcept;
  MossGardenStateSnapshot moss_garden_state() const noexcept;

  bool invariants_hold() const noexcept;
  std::uint64_t state_hash() const noexcept;

private:
  void initialize_fixture() noexcept;
  void initialize_dynamics_fixture() noexcept;
  void relocate_fixture_marker() noexcept;
  void step_determinism_fixture(const InputFrame& frame) noexcept;

  ModelConfig config_{};
  World world_{};
  Pcg32 prng_{};
  std::uint64_t tick_ = 0;

  WorkBudget event_budget_{};
  WorkBudget reaction_budget_{};
  DynamicsEngine dynamics_engine_{};
  LavaWaterScene lava_water_scene_{};
  SodiumWaterScene sodium_water_scene_{};
  OilFireScene oil_fire_scene_{};
  MossGardenScene moss_garden_scene_{};

  FixtureStateSnapshot fixture_{};
  DynamicsStats dynamics_stats_{};
  LavaWaterSceneStats lava_water_stats_{};
  SodiumWaterSceneStats sodium_water_stats_{};
  OilFireSceneStats oil_fire_stats_{};
  MossGardenSceneStats moss_garden_stats_{};
};

} // namespace espsand::sim
