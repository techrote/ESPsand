#include <unity.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/model.hpp>
#include <espsand/sim/prng.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace {

using espsand::sim::BootEvent;
using espsand::sim::Cell;
using espsand::sim::InputFrame;
using espsand::sim::MaterialId;
using espsand::sim::Model;
using espsand::sim::ModelConfig;
using espsand::sim::Pcg32;
using espsand::sim::WorkBudget;
using espsand::sim::World;

InputFrame replay_frame(std::uint32_t tick) {
  InputFrame frame{};
  const int signed_phase = static_cast<int>(tick % 5U) - 2;
  frame.gravity.x = static_cast<float>(signed_phase) * 0.5F;
  frame.gravity.y = -frame.gravity.x;
  frame.gravity_magnitude = 0.8F;
  frame.gravity_confidence = 0.9F;
  frame.shake_energy = static_cast<float>(tick % 7U) / 6.0F;
  frame.motion_energy = static_cast<float>(tick % 9U) / 8.0F;
  frame.spin_rate = static_cast<float>(static_cast<int>(tick % 3U) - 1);

  frame.cap_combo = static_cast<float>(tick % 11U) / 10.0F;
  frame.cap_combo_event = tick % 13U == 0U;

  frame.slider_active = tick % 3U == 0U;
  frame.slider_position = static_cast<float>(tick % 16U) / 15.0F;
  frame.slider_strength = frame.slider_active ? 0.75F : 0.0F;

  frame.noise_event = tick % 7U == 0U;
  frame.noise_impulse = frame.noise_event ? 0.35F : 0.0F;
  frame.tap_impulse = tick % 11U == 0U ? 1.0F : 0.0F;
  return frame;
}

void test_material_registry_is_stable_and_centralized() {
  using namespace espsand::sim;

  TEST_ASSERT_EQUAL_UINT32(1U, kMaterialRegistryVersion);
  TEST_ASSERT_EQUAL_UINT32(12U, kMaterialCount);

  for (std::size_t index = 0; index < kMaterialCount; ++index) {
    const auto id = static_cast<MaterialId>(index);
    const auto* info = material_info(id);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_UINT8(index, static_cast<std::uint8_t>(info->id));
  }

  TEST_ASSERT_EQUAL_STRING("empty", material_info(MaterialId::kEmpty)->name);
  TEST_ASSERT_EQUAL_STRING("water", material_info(MaterialId::kWater)->name);
  TEST_ASSERT_EQUAL_STRING("oil", material_info(MaterialId::kOil)->name);
  TEST_ASSERT_EQUAL_STRING("lava", material_info(MaterialId::kLava)->name);
  TEST_ASSERT_EQUAL_STRING("steam", material_info(MaterialId::kSteam)->name);
  TEST_ASSERT_EQUAL_STRING("smoke", material_info(MaterialId::kSmoke)->name);
  TEST_ASSERT_EQUAL_STRING("fire", material_info(MaterialId::kFire)->name);
  TEST_ASSERT_EQUAL_STRING("sodium_like", material_info(MaterialId::kSodiumLike)->name);
  TEST_ASSERT_EQUAL_STRING("tracer", material_info(MaterialId::kTracer)->name);
  TEST_ASSERT_EQUAL_STRING("moss", material_info(MaterialId::kMoss)->name);
  TEST_ASSERT_NULL(material_info(static_cast<MaterialId>(255U)));
}

void test_world_capacity_order_and_bounds_are_fixed() {
  using namespace espsand::sim;

  TEST_ASSERT_EQUAL_UINT32(16U, kWorldWidth);
  TEST_ASSERT_EQUAL_UINT32(16U, kWorldHeight);
  TEST_ASSERT_EQUAL_UINT32(256U, kWorldCapacity);
  TEST_ASSERT_EQUAL_UINT32(8U, sizeof(Cell));

  World world;
  TEST_ASSERT_TRUE(world.in_bounds(0, 0));
  TEST_ASSERT_TRUE(world.in_bounds(15, 15));
  TEST_ASSERT_FALSE(world.in_bounds(-1, 0));
  TEST_ASSERT_FALSE(world.in_bounds(0, -1));
  TEST_ASSERT_FALSE(world.in_bounds(16, 0));
  TEST_ASSERT_FALSE(world.in_bounds(0, 16));

  TEST_ASSERT_NOT_NULL(world.try_cell(0, 0));
  TEST_ASSERT_NOT_NULL(world.try_cell(15, 15));
  TEST_ASSERT_NULL(world.try_cell(-1, 0));
  TEST_ASSERT_NULL(world.try_cell(0, -1));
  TEST_ASSERT_NULL(world.try_cell(16, 0));
  TEST_ASSERT_NULL(world.try_cell(0, 16));
  TEST_ASSERT_NOT_NULL(world.try_cell_index(255));
  TEST_ASSERT_NULL(world.try_cell_index(256));

  TEST_ASSERT_TRUE(world.try_cell(0, 1) == world.try_cell_index(16));

  Cell moss{};
  moss.material = MaterialId::kMoss;
  moss.mass = 7;
  TEST_ASSERT_TRUE(world.set_cell(1, 0, moss));
  TEST_ASSERT_EQUAL_UINT8(static_cast<std::uint8_t>(MaterialId::kMoss),
                          static_cast<std::uint8_t>(world.cells()[1].material));
  TEST_ASSERT_FALSE(world.set_cell(-1, 0, moss));
  TEST_ASSERT_FALSE(world.set_cell(16, 0, moss));
}

void test_material_accounting_and_invariant_detection() {
  using namespace espsand::sim;

  World world;

  Cell water{};
  water.material = MaterialId::kWater;
  water.mass = 100;
  TEST_ASSERT_TRUE(world.set_cell(1, 1, water));

  water.mass = 50;
  TEST_ASSERT_TRUE(world.set_cell(2, 1, water));

  Cell wall{};
  wall.material = MaterialId::kWall;
  wall.mass = 255;
  TEST_ASSERT_TRUE(world.set_cell(0, 0, wall));

  const auto totals = world.totals();
  TEST_ASSERT_EQUAL_UINT16(2U, totals.cell_count[material_index(MaterialId::kWater)]);
  TEST_ASSERT_EQUAL_UINT32(150U, totals.mass[material_index(MaterialId::kWater)]);
  TEST_ASSERT_EQUAL_UINT16(1U, totals.cell_count[material_index(MaterialId::kWall)]);
  TEST_ASSERT_EQUAL_UINT32(255U, totals.mass[material_index(MaterialId::kWall)]);
  TEST_ASSERT_EQUAL_UINT16(253U, totals.cell_count[material_index(MaterialId::kEmpty)]);
  TEST_ASSERT_EQUAL_UINT16(3U, totals.occupied_cells);
  TEST_ASSERT_EQUAL_UINT32(405U, totals.total_mass);
  TEST_ASSERT_EQUAL_UINT16(0U, totals.invalid_cells);
  TEST_ASSERT_TRUE(world.invariants_hold());

  Cell invalid{};
  invalid.material = static_cast<MaterialId>(250U);
  invalid.mass = 10;
  TEST_ASSERT_FALSE(world.set_cell(3, 3, invalid));

  Cell* direct = world.try_cell(3, 3);
  TEST_ASSERT_NOT_NULL(direct);
  *direct = invalid;

  const auto invalid_totals = world.totals();
  TEST_ASSERT_EQUAL_UINT16(1U, invalid_totals.invalid_cells);
  TEST_ASSERT_EQUAL_UINT32(10U, invalid_totals.invalid_mass);
  TEST_ASSERT_FALSE(world.invariants_hold());
}

void test_work_budget_has_atomic_predictable_saturation() {
  WorkBudget budget(2);
  TEST_ASSERT_TRUE(budget.try_consume());
  TEST_ASSERT_TRUE(budget.try_consume());
  TEST_ASSERT_FALSE(budget.try_consume());
  TEST_ASSERT_EQUAL_UINT16(2U, budget.stats().used);
  TEST_ASSERT_EQUAL_UINT16(1U, budget.stats().dropped);
  TEST_ASSERT_EQUAL_UINT16(0U, budget.remaining());

  budget.reset(1);
  TEST_ASSERT_FALSE(budget.try_consume(2));
  TEST_ASSERT_EQUAL_UINT16(0U, budget.stats().used);
  TEST_ASSERT_EQUAL_UINT16(2U, budget.stats().dropped);
  TEST_ASSERT_EQUAL_UINT16(1U, budget.remaining());

  TEST_ASSERT_TRUE(budget.try_consume(0));
  TEST_ASSERT_EQUAL_UINT16(0U, budget.stats().used);
}

void test_prng_sequence_is_locked() {
  // clang-format off
  constexpr std::uint32_t expected[] = {
      2707161783U,
      2068313097U,
      3122475824U,
      2211639955U,
      3215226955U,
      3421331566U,
  };
  // clang-format on

  Pcg32 prng(42U);
  TEST_ASSERT_EQUAL_UINT64(109U, prng.increment());

  for (std::uint32_t value : expected) {
    TEST_ASSERT_EQUAL_UINT32(value, prng.next_u32());
  }
}

void test_same_seed_and_inputs_replay_identically() {
  ModelConfig config{};
  config.seed = 0xBADC0FFEE0DDF00DULL;
  config.event_budget = 3;
  config.reaction_budget = 4;

  Model first(config);
  Model second(config);
  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());

  for (std::uint32_t tick = 0; tick < 256U; ++tick) {
    const InputFrame frame = replay_frame(tick);
    first.step(frame);
    second.step(frame);
    TEST_ASSERT_TRUE(first.invariants_hold());
    TEST_ASSERT_TRUE(second.invariants_hold());
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  }
}

void test_golden_state_trace_is_locked() {
  // clang-format off
  constexpr std::uint64_t expected[] = {
      0x4943A6C732CA020DULL,
      0x75A4B3C9249EF546ULL,
      0xFC14CC3D7A9C7408ULL,
      0xB411D621F3D3F1C6ULL,
      0x8F0F30D22E87FB14ULL,
  };
  // clang-format on

  ModelConfig config{};
  config.seed = 0x0123456789ABCDEFULL;
  config.event_budget = 2;
  config.reaction_budget = 3;
  Model model(config);

  TEST_ASSERT_EQUAL_UINT64(expected[0], model.state_hash());

  model.step(InputFrame{});
  TEST_ASSERT_EQUAL_UINT64(expected[1], model.state_hash());

  InputFrame frame{};
  frame.cap_combo = 0.4F;
  frame.slider_active = true;
  frame.slider_position = 0.25F;
  frame.slider_strength = 0.8F;
  frame.noise_impulse = 0.5F;
  frame.noise_event = true;
  model.step(frame);
  TEST_ASSERT_EQUAL_UINT64(expected[2], model.state_hash());

  frame = InputFrame{};
  frame.tap_impulse = 1.0F;
  model.step(frame);
  TEST_ASSERT_EQUAL_UINT64(expected[3], model.state_hash());

  frame = InputFrame{};
  frame.cap_combo = 0.9F;
  frame.cap_combo_event = true;
  frame.slider_active = true;
  frame.slider_position = 0.75F;
  frame.slider_strength = 1.0F;
  frame.noise_impulse = 0.25F;
  frame.noise_event = true;
  frame.tap_impulse = 1.0F;
  model.step(frame);
  TEST_ASSERT_EQUAL_UINT64(expected[4], model.state_hash());

  const auto work = model.tick_work_stats();
  TEST_ASSERT_EQUAL_UINT16(2U, work.events.used);
  TEST_ASSERT_EQUAL_UINT16(1U, work.events.dropped);
  TEST_ASSERT_EQUAL_UINT16(0U, work.reactions.used);
}

void test_different_seeds_control_stochastic_fixture() {
  ModelConfig first_config{};
  first_config.seed = 1;
  ModelConfig second_config{};
  second_config.seed = 2;

  Model first(first_config);
  Model second(second_config);

  const auto first_fixture = first.fixture_state();
  const auto second_fixture = second.fixture_state();

  TEST_ASSERT_TRUE(first_fixture.marker_x != second_fixture.marker_x ||
                   first_fixture.marker_y != second_fixture.marker_y);
  TEST_ASSERT_TRUE(first.state_hash() != second.state_hash());
}

void test_reset_and_reseed_are_deterministic() {
  ModelConfig config{};
  config.seed = 123;
  Model model(config);

  const std::uint64_t initial_hash = model.state_hash();
  const auto initial_fixture = model.fixture_state();

  InputFrame frame{};
  frame.tap_impulse = 1.0F;
  frame.noise_impulse = 1.0F;
  frame.noise_event = true;
  model.step(frame);
  TEST_ASSERT_TRUE(model.state_hash() != initial_hash);

  model.reset();
  TEST_ASSERT_EQUAL_UINT64(initial_hash, model.state_hash());
  TEST_ASSERT_EQUAL_UINT64(0U, model.tick());
  TEST_ASSERT_EQUAL_UINT8(initial_fixture.marker_x, model.fixture_state().marker_x);
  TEST_ASSERT_EQUAL_UINT8(initial_fixture.marker_y, model.fixture_state().marker_y);

  model.reseed(456U);
  const std::uint64_t reseeded_hash = model.state_hash();
  TEST_ASSERT_TRUE(reseeded_hash != initial_hash);
  TEST_ASSERT_EQUAL_UINT64(456U, model.seed());

  model.reset();
  TEST_ASSERT_EQUAL_UINT64(reseeded_hash, model.state_hash());
}

void test_external_touch_noise_does_not_mutate_prng_state() {
  ModelConfig config{};
  config.seed = 0xA55A5AA5U;
  Model model(config);

  const std::uint64_t state_before = model.prng().state();
  const std::uint64_t increment_before = model.prng().increment();

  InputFrame frame{};
  frame.cap_combo = 0.9F;
  frame.cap_combo_event = true;
  frame.slider_active = true;
  frame.slider_position = 0.75F;
  frame.slider_strength = 1.0F;
  frame.noise_impulse = 1.0F;
  frame.noise_event = true;
  model.step(frame);

  TEST_ASSERT_EQUAL_UINT64(state_before, model.prng().state());
  TEST_ASSERT_EQUAL_UINT64(increment_before, model.prng().increment());
  TEST_ASSERT_TRUE(model.fixture_state().external_impulse > 0U);

  frame = InputFrame{};
  frame.tap_impulse = 1.0F;
  model.step(frame);
  TEST_ASSERT_TRUE(model.prng().state() != state_before);
}

void test_input_sanitization_clamps_malformed_values() {
  using espsand::sim::sanitize_input_frame;

  InputFrame input{};
  input.gravity.x = std::numeric_limits<float>::quiet_NaN();
  input.gravity.y = std::numeric_limits<float>::infinity();
  input.gravity_magnitude = -100.0F;
  input.gravity_confidence = 100.0F;
  input.shake_energy = 2.0F;
  input.motion_energy = -1.0F;
  input.tap_impulse = 1.5F;
  input.spin_rate = -3.0F;
  input.cap_combo = 9.0F;
  input.slider_position = std::numeric_limits<float>::quiet_NaN();
  input.slider_strength = -1.0F;
  input.noise_impulse = std::numeric_limits<float>::infinity();
  input.boot_event = static_cast<BootEvent>(255U);

  const InputFrame output = sanitize_input_frame(input);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, output.gravity.x);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, output.gravity.y);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, output.gravity_magnitude);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 1.0F, output.gravity_confidence);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 1.0F, output.shake_energy);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, output.motion_energy);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 1.0F, output.tap_impulse);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, -1.0F, output.spin_rate);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 1.0F, output.cap_combo);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.5F, output.slider_position);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, output.slider_strength);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, output.noise_impulse);
  TEST_ASSERT_EQUAL_UINT8(static_cast<std::uint8_t>(BootEvent::kNone),
                          static_cast<std::uint8_t>(output.boot_event));
}

void test_randomized_stress_preserves_replay_and_invariants() {
  using namespace espsand::sim;

  ModelConfig config{};
  config.seed = 0xC001D00DU;
  config.event_budget = 2;
  config.reaction_budget = 1;
  Model first(config);
  Model second(config);

  std::uint32_t random = 0x31415926U;
  World bounds_world;

  for (std::uint32_t tick = 0; tick < 5000U; ++tick) {
    auto next = [&random]() {
      random = random * 1664525U + 1013904223U;
      return random;
    };

    InputFrame frame{};
    frame.gravity.x = static_cast<float>(static_cast<std::int32_t>(next())) / 536870912.0F;
    frame.gravity.y = static_cast<float>(static_cast<std::int32_t>(next())) / 536870912.0F;
    frame.gravity_magnitude = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.gravity_confidence = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.shake_energy = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.motion_energy = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.tap_impulse = (next() & 0x1FU) == 0U ? 1.0F : 0.0F;
    frame.spin_rate = static_cast<float>(static_cast<std::int32_t>(next())) / 536870912.0F;
    frame.cap_combo = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.cap_combo_event = (next() & 0x3FU) == 0U;
    frame.slider_active = (next() & 0x7U) == 0U;
    frame.slider_position = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.slider_strength = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.noise_impulse = static_cast<float>(next() & 0x3FFU) / 255.0F;
    frame.noise_event = (next() & 0xFU) == 0U;

    if (tick % 257U == 0U) {
      frame.slider_position = std::numeric_limits<float>::quiet_NaN();
      frame.noise_impulse = std::numeric_limits<float>::infinity();
    }

    first.step(frame);
    second.step(frame);

    TEST_ASSERT_TRUE(first.invariants_hold());
    TEST_ASSERT_TRUE(second.invariants_hold());

    const auto totals = first.world().totals();
    std::uint32_t accounted_cells = totals.invalid_cells;
    for (std::uint16_t count : totals.cell_count) {
      accounted_cells += count;
    }
    TEST_ASSERT_EQUAL_UINT32(kWorldCapacity, accounted_cells);

    const auto work = first.tick_work_stats();
    TEST_ASSERT_TRUE(work.events.used <= work.events.limit);
    TEST_ASSERT_TRUE(work.reactions.used <= work.reactions.limit);

    if (tick % 97U == 0U) {
      TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
    }

    const int x = static_cast<int>(next() % 96U) - 40;
    const int y = static_cast<int>(next() % 96U) - 40;
    if (bounds_world.in_bounds(x, y)) {
      TEST_ASSERT_NOT_NULL(bounds_world.try_cell(x, y));
    } else {
      TEST_ASSERT_NULL(bounds_world.try_cell(x, y));
      Cell cell{};
      cell.material = MaterialId::kWater;
      TEST_ASSERT_FALSE(bounds_world.set_cell(x, y, cell));
    }
  }

  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  TEST_ASSERT_EQUAL_UINT64(5000U, first.tick());
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_material_registry_is_stable_and_centralized);
  RUN_TEST(test_world_capacity_order_and_bounds_are_fixed);
  RUN_TEST(test_material_accounting_and_invariant_detection);
  RUN_TEST(test_work_budget_has_atomic_predictable_saturation);
  RUN_TEST(test_prng_sequence_is_locked);
  RUN_TEST(test_same_seed_and_inputs_replay_identically);
  RUN_TEST(test_golden_state_trace_is_locked);
  RUN_TEST(test_different_seeds_control_stochastic_fixture);
  RUN_TEST(test_reset_and_reseed_are_deterministic);
  RUN_TEST(test_external_touch_noise_does_not_mutate_prng_state);
  RUN_TEST(test_input_sanitization_clamps_malformed_values);
  RUN_TEST(test_randomized_stress_preserves_replay_and_invariants);
  return UNITY_END();
}
