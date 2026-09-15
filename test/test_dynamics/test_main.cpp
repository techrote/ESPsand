#include <unity.h>

#include <algorithm>
#include <cstdint>

#include <espsand/sim/dynamics.hpp>
#include <espsand/sim/model.hpp>
#include <espsand/sim/world.hpp>

namespace {

using espsand::sim::Cell;
using espsand::sim::DynamicsEngine;
using espsand::sim::InputFrame;
using espsand::sim::MaterialId;
using espsand::sim::Model;
using espsand::sim::ModelConfig;
using espsand::sim::SceneId;
using espsand::sim::WorkBudget;
using espsand::sim::World;

Cell make_cell(MaterialId material, std::uint8_t mass, std::int16_t temperature = 0,
               std::uint8_t aux = 0) {
  Cell cell{};
  cell.material = material;
  cell.mass = mass;
  cell.temperature = temperature;
  cell.aux = aux;
  return cell;
}

InputFrame gravity_frame(float x, float y) {
  InputFrame frame{};
  frame.gravity.x = x;
  frame.gravity.y = y;
  frame.gravity_magnitude = 1.0F;
  frame.gravity_confidence = 1.0F;
  return frame;
}

void test_transport_moves_with_gravity_and_conserves_mass() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(8, 5, make_cell(MaterialId::kWater, 200)));
  const std::uint32_t initial_mass = world.totals().total_mass;

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, gravity_frame(0.0F, 1.0F), 0, events, reactions);

  TEST_ASSERT_EQUAL_UINT32(initial_mass, world.totals().total_mass);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kEmpty),
                        static_cast<int>(world.try_cell(8, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kWater),
                        static_cast<int>(world.try_cell(8, 6)->material));
  TEST_ASSERT_TRUE(stats.transport_moves >= 1U);
  TEST_ASSERT_TRUE(world.invariants_hold());
}

void test_rotated_gravity_moves_water_right() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(5, 8, make_cell(MaterialId::kWater, 180)));

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, gravity_frame(1.0F, 0.0F), 0, events, reactions);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kWater),
                        static_cast<int>(world.try_cell(6, 8)->material));
  TEST_ASSERT_EQUAL_INT8(1, stats.gravity_dx);
  TEST_ASSERT_EQUAL_INT8(0, stats.gravity_dy);
}

void test_density_orders_water_below_oil() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(5, 5, make_cell(MaterialId::kWater, 220)));
  TEST_ASSERT_TRUE(world.set_cell(5, 6, make_cell(MaterialId::kOil, 200)));
  TEST_ASSERT_TRUE(world.set_cell(4, 6, make_cell(MaterialId::kCrust, 255)));
  TEST_ASSERT_TRUE(world.set_cell(6, 6, make_cell(MaterialId::kCrust, 255)));
  TEST_ASSERT_TRUE(world.set_cell(5, 7, make_cell(MaterialId::kCrust, 255)));

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, gravity_frame(0.0F, 1.0F), 0, events, reactions);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kOil),
                        static_cast<int>(world.try_cell(5, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kWater),
                        static_cast<int>(world.try_cell(5, 6)->material));
  TEST_ASSERT_TRUE(stats.density_swaps >= 1U);
}

void test_gas_rises_through_water() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(4, 5, make_cell(MaterialId::kWater, 220)));
  TEST_ASSERT_TRUE(world.set_cell(4, 6, make_cell(MaterialId::kSteam, 100, 700)));

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, gravity_frame(0.0F, 1.0F), 0, events, reactions);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kSteam),
                        static_cast<int>(world.try_cell(4, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kWater),
                        static_cast<int>(world.try_cell(4, 6)->material));
  TEST_ASSERT_TRUE(stats.gas_moves >= 1U);
}

void test_material_dynamics_distinguishes_liquid_mobility() {
  const auto* water = espsand::sim::material_dynamics_info(MaterialId::kWater);
  const auto* oil = espsand::sim::material_dynamics_info(MaterialId::kOil);
  const auto* lava = espsand::sim::material_dynamics_info(MaterialId::kLava);

  TEST_ASSERT_NOT_NULL(water);
  TEST_ASSERT_NOT_NULL(oil);
  TEST_ASSERT_NOT_NULL(lava);
  TEST_ASSERT_TRUE(water->gravity_mobility > oil->gravity_mobility);
  TEST_ASSERT_TRUE(oil->gravity_mobility > lava->gravity_mobility);
  TEST_ASSERT_TRUE(water->lateral_mobility > lava->lateral_mobility);
  TEST_ASSERT_TRUE(water->density > oil->density);
  TEST_ASSERT_TRUE(lava->density > water->density);
}

void test_heat_exchange_converges_and_ambient_loss_is_bounded() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(5, 5, make_cell(MaterialId::kWater, 200, 1000)));
  TEST_ASSERT_TRUE(world.set_cell(6, 5, make_cell(MaterialId::kWater, 200, 0)));

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, InputFrame{}, 0, events, reactions);

  const std::int16_t hot = world.try_cell(5, 5)->temperature;
  const std::int16_t cold = world.try_cell(6, 5)->temperature;
  TEST_ASSERT_TRUE(hot < 1000);
  TEST_ASSERT_TRUE(hot > cold);
  TEST_ASSERT_TRUE(cold > 0);
  TEST_ASSERT_TRUE(static_cast<std::int32_t>(hot) + cold <= 1000);
  TEST_ASSERT_TRUE(stats.heat_pairs >= 1U);
}

void test_reaction_budget_caps_products_without_recursive_runaway() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(2, 5, make_cell(MaterialId::kLava, 200, 1200)));
  TEST_ASSERT_TRUE(world.set_cell(3, 5, make_cell(MaterialId::kWater, 180, 20)));
  TEST_ASSERT_TRUE(world.set_cell(6, 5, make_cell(MaterialId::kLava, 200, 1200)));
  TEST_ASSERT_TRUE(world.set_cell(7, 5, make_cell(MaterialId::kWater, 180, 20)));
  const std::uint32_t initial_mass = world.totals().total_mass;

  WorkBudget events(16);
  WorkBudget reactions(1);
  const auto stats = DynamicsEngine{}.step(world, InputFrame{}, 0, events, reactions);

  TEST_ASSERT_EQUAL_UINT16(1U, reactions.stats().used);
  TEST_ASSERT_TRUE(reactions.stats().dropped >= 1U);
  TEST_ASSERT_EQUAL_UINT16(1U, stats.reactions_applied);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kCrust),
                        static_cast<int>(world.try_cell(2, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kSteam),
                        static_cast<int>(world.try_cell(3, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kLava),
                        static_cast<int>(world.try_cell(6, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kWater),
                        static_cast<int>(world.try_cell(7, 5)->material));
  TEST_ASSERT_EQUAL_UINT32(initial_mass, world.totals().total_mass);
}

void test_sodium_like_water_reaction_is_finite_shared_primitive() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(2, 2, make_cell(MaterialId::kSodiumLike, 80, 50)));
  TEST_ASSERT_TRUE(world.set_cell(3, 2, make_cell(MaterialId::kWater, 160, 20)));

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, InputFrame{}, 0, events, reactions);

  const Cell* product = world.try_cell(2, 2);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kFire), static_cast<int>(product->material));
  TEST_ASSERT_TRUE(product->aux >= 12U);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kSteam),
                        static_cast<int>(world.try_cell(3, 2)->material));
  TEST_ASSERT_EQUAL_UINT16(1U, stats.reactions_applied);
}

void test_oil_fire_reaction_consumes_oil_state() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(5, 5, make_cell(MaterialId::kOil, 180, 20)));
  TEST_ASSERT_TRUE(world.set_cell(6, 5, make_cell(MaterialId::kFire, 80, 900, 10)));

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, InputFrame{}, 0, events, reactions);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kFire),
                        static_cast<int>(world.try_cell(5, 5)->material));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kFire),
                        static_cast<int>(world.try_cell(6, 5)->material));
  TEST_ASSERT_EQUAL_UINT16(1U, stats.reactions_applied);
}

void test_fire_lifetime_expires_to_smoke_without_mass_loss() {
  World world;
  TEST_ASSERT_TRUE(world.set_cell(5, 5, make_cell(MaterialId::kFire, 80, 900, 1)));
  const std::uint32_t initial_mass = world.totals().total_mass;

  WorkBudget events(16);
  WorkBudget reactions(16);
  const auto stats = DynamicsEngine{}.step(world, InputFrame{}, 0, events, reactions);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(MaterialId::kSmoke),
                        static_cast<int>(world.try_cell(5, 5)->material));
  TEST_ASSERT_EQUAL_UINT16(1U, stats.fire_cells_expired);
  TEST_ASSERT_EQUAL_UINT32(initial_mass, world.totals().total_mass);
}

void test_motion_disturbance_does_not_change_gravity_direction() {
  World calm_world;
  World shaken_world;
  WorkBudget calm_events(16);
  WorkBudget calm_reactions(16);
  WorkBudget shaken_events(16);
  WorkBudget shaken_reactions(16);

  InputFrame calm = gravity_frame(-1.0F, 0.0F);
  InputFrame shaken = calm;
  shaken.shake_energy = 1.0F;
  shaken.motion_energy = 0.8F;
  shaken.tap_impulse = 0.7F;

  DynamicsEngine engine;
  const auto calm_stats = engine.step(calm_world, calm, 0, calm_events, calm_reactions);
  const auto shaken_stats =
      engine.step(shaken_world, shaken, 0, shaken_events, shaken_reactions);

  TEST_ASSERT_EQUAL_INT8(calm_stats.gravity_dx, shaken_stats.gravity_dx);
  TEST_ASSERT_EQUAL_INT8(calm_stats.gravity_dy, shaken_stats.gravity_dy);
  TEST_ASSERT_EQUAL_INT8(-1, calm_stats.gravity_dx);
  TEST_ASSERT_TRUE(shaken_stats.disturbance_q8 > calm_stats.disturbance_q8);
}

InputFrame replay_frame(std::uint32_t tick) {
  InputFrame frame{};
  switch (tick % 4U) {
  case 0:
    frame = gravity_frame(0.0F, 1.0F);
    break;
  case 1:
    frame = gravity_frame(1.0F, 0.0F);
    break;
  case 2:
    frame = gravity_frame(0.0F, -1.0F);
    break;
  default:
    frame = gravity_frame(-1.0F, 0.0F);
    break;
  }
  frame.shake_energy = static_cast<float>(tick % 7U) / 6.0F;
  frame.motion_energy = static_cast<float>(tick % 5U) / 4.0F;
  frame.tap_impulse = tick % 13U == 0U ? 1.0F : 0.0F;
  frame.spin_rate = static_cast<float>(static_cast<int>(tick % 3U) - 1) * 0.5F;
  return frame;
}

void test_dynamics_model_replay_is_deterministic() {
  ModelConfig config{};
  config.seed = 0x600DCAFEU;
  config.scene = SceneId::kDynamicsFixture;
  config.event_budget = 4;
  config.reaction_budget = 3;

  Model first(config);
  Model second(config);
  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());

  for (std::uint32_t tick = 0; tick < 256U; ++tick) {
    const InputFrame frame = replay_frame(tick);
    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
    TEST_ASSERT_TRUE(first.invariants_hold());
    TEST_ASSERT_TRUE(second.invariants_hold());
  }
}

void test_dynamics_model_reset_restores_initial_state() {
  ModelConfig config{};
  config.seed = 12345;
  config.scene = SceneId::kDynamicsFixture;
  Model model(config);
  const std::uint64_t initial_hash = model.state_hash();

  for (std::uint32_t tick = 0; tick < 20U; ++tick) {
    model.step(replay_frame(tick));
  }
  TEST_ASSERT_TRUE(model.state_hash() != initial_hash);

  model.reset();
  TEST_ASSERT_EQUAL_UINT64(initial_hash, model.state_hash());
  TEST_ASSERT_EQUAL_UINT64(0U, model.tick());
}

void test_randomized_dynamics_stress_preserves_mass_and_bounds() {
  std::uint32_t state = 0xC001D00DU;
  auto next = [&state]() {
    state = state * 1664525U + 1013904223U;
    return state;
  };

  World world;
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      const MaterialId material =
          static_cast<MaterialId>(next() % static_cast<std::uint32_t>(MaterialId::kCount));
      const std::uint8_t mass = material == MaterialId::kEmpty
                                    ? 0U
                                    : static_cast<std::uint8_t>(1U + (next() % 255U));
      const std::int16_t temperature =
          static_cast<std::int16_t>(static_cast<std::int32_t>(next() % 4001U) - 2000);
      TEST_ASSERT_TRUE(world.set_cell(x, y, make_cell(material, mass, temperature,
                                                     static_cast<std::uint8_t>(next() >> 24U))));
    }
  }

  const std::uint32_t initial_mass = world.totals().total_mass;
  DynamicsEngine engine;
  for (std::uint32_t tick = 0; tick < 2000U; ++tick) {
    InputFrame frame{};
    const int axis = static_cast<int>(next() % 4U);
    frame = axis == 0   ? gravity_frame(1.0F, 0.0F)
            : axis == 1 ? gravity_frame(-1.0F, 0.0F)
            : axis == 2 ? gravity_frame(0.0F, 1.0F)
                        : gravity_frame(0.0F, -1.0F);
    frame.shake_energy = static_cast<float>(next() & 0xFFU) / 255.0F;
    frame.motion_energy = static_cast<float>(next() & 0xFFU) / 255.0F;
    frame.tap_impulse = static_cast<float>(next() & 0xFFU) / 255.0F;
    frame.spin_rate = static_cast<float>(static_cast<int>(next() & 0xFFU) - 128) / 128.0F;

    WorkBudget events(12);
    WorkBudget reactions(8);
    static_cast<void>(engine.step(world, frame, tick, events, reactions));

    TEST_ASSERT_TRUE(world.invariants_hold());
    TEST_ASSERT_EQUAL_UINT32(initial_mass, world.totals().total_mass);
    TEST_ASSERT_TRUE(events.stats().used <= events.stats().limit);
    TEST_ASSERT_TRUE(reactions.stats().used <= reactions.stats().limit);
  }
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_transport_moves_with_gravity_and_conserves_mass);
  RUN_TEST(test_rotated_gravity_moves_water_right);
  RUN_TEST(test_density_orders_water_below_oil);
  RUN_TEST(test_gas_rises_through_water);
  RUN_TEST(test_material_dynamics_distinguishes_liquid_mobility);
  RUN_TEST(test_heat_exchange_converges_and_ambient_loss_is_bounded);
  RUN_TEST(test_reaction_budget_caps_products_without_recursive_runaway);
  RUN_TEST(test_sodium_like_water_reaction_is_finite_shared_primitive);
  RUN_TEST(test_oil_fire_reaction_consumes_oil_state);
  RUN_TEST(test_fire_lifetime_expires_to_smoke_without_mass_loss);
  RUN_TEST(test_motion_disturbance_does_not_change_gravity_direction);
  RUN_TEST(test_dynamics_model_replay_is_deterministic);
  RUN_TEST(test_dynamics_model_reset_restores_initial_state);
  RUN_TEST(test_randomized_dynamics_stress_preserves_mass_and_bounds);
  return UNITY_END();
}
