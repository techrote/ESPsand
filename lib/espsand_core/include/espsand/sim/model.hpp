#pragma once

#include <cstdint>

#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/prng.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kStateHashSchemaVersion = 1;

enum class SceneId : std::uint8_t {
  kDeterminismFixture = 0,
};

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

  bool invariants_hold() const noexcept;
  std::uint64_t state_hash() const noexcept;

private:
  void initialize_fixture() noexcept;
  void relocate_fixture_marker() noexcept;

  ModelConfig config_{};
  World world_{};
  Pcg32 prng_{};
  std::uint64_t tick_ = 0;

  WorkBudget event_budget_{};
  WorkBudget reaction_budget_{};

  FixtureStateSnapshot fixture_{};
};

} // namespace espsand::sim
