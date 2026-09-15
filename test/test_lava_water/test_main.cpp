#include <unity.h>

#include <cmath>
#include <cstdint>

#include <espsand/input/motion_interpreter.hpp>
#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/model.hpp>
#include <espsand/sim/scene_limits.hpp>
#include <espsand/sim/world.hpp>

namespace {

using espsand::input::MotionInterpreter;
using espsand::input::PlaneTransform;
using espsand::io::ImuSample;
using espsand::render::WorldRenderer;
using espsand::sim::Cell;
using espsand::sim::InputFrame;
using espsand::sim::MaterialId;
using espsand::sim::Model;
using espsand::sim::ModelConfig;
using espsand::sim::SceneId;
using espsand::sim::World;

ModelConfig lava_water_config(std::uint64_t seed = 0xE5007007ULL) {
  ModelConfig config{};
  config.seed = seed;
  config.scene = SceneId::kLavaWater;
  config.event_budget = 12;
  config.reaction_budget = 12;
  return config;
}

InputFrame gravity_frame(float x, float y) {
  InputFrame frame{};
  frame.gravity = {x, y};
  frame.gravity_magnitude = 1.0F;
  frame.gravity_confidence = 1.0F;
  return frame;
}

std::uint32_t material_mass(const Model& model, MaterialId material) {
  return model.world().totals().mass[espsand::sim::material_index(material)];
}

std::uint16_t material_cells(const Model& model, MaterialId material) {
  return model.world().totals().cell_count[espsand::sim::material_index(material)];
}

bool has_material_in_top_column(const Model& model, MaterialId material, std::uint8_t x) {
  for (int y = 0; y <= 2; ++y) {
    const Cell* cell = model.world().try_cell(x, y);
    if (cell != nullptr && cell->material == material && cell->mass != 0U) {
      return true;
    }
  }
  return false;
}

struct Centroid {
  float x = 0.0F;
  float y = 0.0F;
  std::uint32_t count = 0;
};

Centroid centroid_for(const World& world, MaterialId material) {
  Centroid result{};
  for (std::size_t index = 0; index < espsand::sim::kWorldCapacity; ++index) {
    const Cell* cell = world.try_cell_index(index);
    if (cell == nullptr || cell->material != material || cell->mass == 0U) {
      continue;
    }
    result.x += static_cast<float>(index % espsand::sim::kWorldWidth);
    result.y += static_cast<float>(index / espsand::sim::kWorldWidth);
    ++result.count;
  }
  if (result.count != 0U) {
    result.x /= static_cast<float>(result.count);
    result.y /= static_cast<float>(result.count);
  }
  return result;
}

Cell material_cell(MaterialId material, std::uint8_t mass, std::int16_t temperature = 0) {
  Cell cell{};
  cell.material = material;
  cell.mass = mass;
  cell.temperature = temperature;
  return cell;
}

void fill_block(World& world, int base_x, int base_y, const Cell& cell) {
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      TEST_ASSERT_TRUE(world.set_cell(base_x + x, base_y + y, cell));
    }
  }
}

InputFrame trace_frame(std::uint32_t tick) {
  InputFrame frame{};
  switch ((tick / 24U) % 4U) {
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
  if (tick == 25U) {
    frame.shake_energy = 1.0F;
  }
  if (tick == 40U) {
    frame.tap_impulse = 1.0F;
  }
  if (tick == 50U) {
    frame.cap_combo = 1.0F;
    frame.cap_combo_event = true;
  }
  if (tick >= 60U && tick < 66U) {
    frame.slider_active = true;
    frame.slider_position = 0.72F;
    frame.slider_strength = 0.9F;
  }
  return frame;
}

void test_scene_initialization_is_sparse_and_deterministic() {
  Model first(lava_water_config());
  Model second(lava_water_config());

  TEST_ASSERT_TRUE(first.invariants_hold());
  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  TEST_ASSERT_EQUAL_UINT16(12U, material_cells(first, MaterialId::kWater));
  TEST_ASSERT_EQUAL_UINT16(7U, material_cells(first, MaterialId::kLava));
  TEST_ASSERT_TRUE(material_cells(first, MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_TRUE(material_cells(first, MaterialId::kLava) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_EQUAL_UINT32(0U, material_mass(first, MaterialId::kSteam));
}

void test_autonomous_contact_creates_persistent_crust_and_steam() {
  Model model(lava_water_config());
  const std::uint32_t initial_mass = model.world().totals().total_mass;

  for (std::uint32_t tick = 0; tick < 24U; ++tick) {
    model.step(gravity_frame(0.0F, 1.0F));
  }

  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kCrust) > 0U);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kSteam) > 0U);
  TEST_ASSERT_EQUAL_UINT32(initial_mass, model.world().totals().total_mass);
  TEST_ASSERT_TRUE(model.invariants_hold());
}

void test_tilt_materially_changes_flow_geometry() {
  Model downward(lava_water_config(99));
  Model rightward(lava_water_config(99));

  for (std::uint32_t tick = 0; tick < 20U; ++tick) {
    downward.step(gravity_frame(0.0F, 1.0F));
    rightward.step(gravity_frame(1.0F, 0.0F));
  }

  const Centroid down_lava = centroid_for(downward.world(), MaterialId::kLava);
  const Centroid right_lava = centroid_for(rightward.world(), MaterialId::kLava);
  TEST_ASSERT_TRUE(down_lava.count != 0U);
  TEST_ASSERT_TRUE(right_lava.count != 0U);
  TEST_ASSERT_TRUE(std::fabs(down_lava.x - right_lava.x) > 0.5F ||
                   std::fabs(down_lava.y - right_lava.y) > 0.5F);
  TEST_ASSERT_TRUE(downward.state_hash() != rightward.state_hash());
}

void test_shake_fractures_crust_without_mass_loss() {
  Model model(lava_water_config(123));
  for (std::uint32_t tick = 0; tick < 18U; ++tick) {
    model.step(gravity_frame(0.0F, 1.0F));
  }
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kCrust) > 0U);

  const std::uint32_t before_mass = model.world().totals().total_mass;
  InputFrame disturbed = gravity_frame(0.0F, 1.0F);
  disturbed.shake_energy = 1.0F;
  model.step(disturbed);

  TEST_ASSERT_TRUE(model.lava_water_stats().crust_fractures > 0U);
  TEST_ASSERT_EQUAL_UINT32(before_mass, model.world().totals().total_mass);
}

void test_combo_uses_shared_reaction_path_for_bounded_burst() {
  Model model(lava_water_config(444));
  InputFrame frame = gravity_frame(0.0F, 1.0F);
  frame.cap_combo = 1.0F;
  frame.cap_combo_event = true;
  model.step(frame);

  TEST_ASSERT_EQUAL_UINT16(1U, model.lava_water_stats().burst_pairs);
  TEST_ASSERT_TRUE(model.tick_work_stats().events.used <= model.tick_work_stats().events.limit);
  TEST_ASSERT_TRUE(model.tick_work_stats().reactions.used <=
                   model.tick_work_stats().reactions.limit);

  bool saw_reaction = model.dynamics_stats().reactions_applied != 0U;
  for (std::uint32_t tick = 0; tick < 60U && !saw_reaction; ++tick) {
    model.step(gravity_frame(0.0F, 1.0F));
    saw_reaction = model.dynamics_stats().reactions_applied != 0U;
  }
  TEST_ASSERT_TRUE(saw_reaction);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kCrust) > 0U);
}

void test_pinch_slider_spawns_secondary_water_near_selected_x() {
  Model model(lava_water_config(555));
  InputFrame frame{};
  frame.slider_active = true;
  frame.slider_position = 0.75F;
  frame.slider_strength = 0.9F;
  model.step(frame);

  const auto stats = model.lava_water_stats();
  TEST_ASSERT_EQUAL_UINT16(1U, stats.touch_water_injections);
  TEST_ASSERT_TRUE(stats.last_injection_x >= 8U);
  TEST_ASSERT_TRUE(stats.last_injection_x <= 15U);
  TEST_ASSERT_TRUE(has_material_in_top_column(model, MaterialId::kWater, stats.last_injection_x));
  TEST_ASSERT_TRUE(material_cells(model, MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);
}

void test_reset_restores_exact_fixed_seed_scene() {
  Model model(lava_water_config(0xABCDEFULL));
  const std::uint64_t initial_hash = model.state_hash();
  for (std::uint32_t tick = 0; tick < 80U; ++tick) {
    model.step(trace_frame(tick));
  }
  TEST_ASSERT_TRUE(model.state_hash() != initial_hash);
  model.reset();
  TEST_ASSERT_EQUAL_UINT64(initial_hash, model.state_hash());
}

void test_fixed_seed_trace_replays_identically() {
  Model first(lava_water_config(0x12345678ULL));
  Model second(lava_water_config(0x12345678ULL));

  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  for (std::uint32_t tick = 0; tick < 96U; ++tick) {
    const InputFrame frame = trace_frame(tick);
    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  }
}

void test_renderer_keeps_lava_water_crust_and_steam_distinct() {
  World world;
  fill_block(world, 0, 0, material_cell(MaterialId::kWater, 220, 20));
  fill_block(world, 2, 0, material_cell(MaterialId::kLava, 230, 1500));
  fill_block(world, 4, 0, material_cell(MaterialId::kCrust, 230, 100));
  fill_block(world, 6, 0, material_cell(MaterialId::kSteam, 180, 650));

  const auto frame = WorldRenderer{}.render(world).frame;
  const auto water = frame[0];
  const auto lava = frame[1];
  const auto crust = frame[2];
  const auto steam = frame[3];
  TEST_ASSERT_TRUE(water.b > water.r && water.b > water.g);
  TEST_ASSERT_TRUE(lava.r > lava.g && lava.g > lava.b);
  TEST_ASSERT_TRUE(crust.r > crust.g && crust.r > crust.b);
  TEST_ASSERT_TRUE(steam.b >= steam.r && steam.b >= steam.g);
  const std::uint32_t lava_energy = lava.r + lava.g + lava.b;
  const std::uint32_t crust_energy = crust.r + crust.g + crust.b;
  TEST_ASSERT_TRUE(crust_energy < lava_energy);
}

void test_motion_interpreter_produces_stable_gravity_without_false_shake() {
  MotionInterpreter interpreter;
  const PlaneTransform identity{};
  for (std::uint32_t sample_index = 0; sample_index < 80U; ++sample_index) {
    ImuSample sample{};
    sample.valid = true;
    sample.timestamp_us = static_cast<std::uint64_t>(sample_index) * 5000U;
    sample.accel_g = {0.0F, 1.0F, 0.0F};
    interpreter.update(sample, identity);
  }

  const auto snapshot = interpreter.snapshot();
  TEST_ASSERT_TRUE(snapshot.ready);
  TEST_ASSERT_FLOAT_WITHIN(0.05F, 0.0F, snapshot.gravity.x);
  TEST_ASSERT_FLOAT_WITHIN(0.05F, 1.0F, snapshot.gravity.y);
  TEST_ASSERT_TRUE(snapshot.shake_energy < 0.05F);
}

void test_motion_impulse_is_separate_from_low_pass_gravity() {
  MotionInterpreter interpreter;
  const PlaneTransform identity{};
  for (std::uint32_t sample_index = 0; sample_index < 40U; ++sample_index) {
    ImuSample sample{};
    sample.valid = true;
    sample.timestamp_us = static_cast<std::uint64_t>(sample_index) * 5000U;
    sample.accel_g = {0.0F, 1.0F, 0.0F};
    interpreter.update(sample, identity);
  }

  ImuSample impulse{};
  impulse.valid = true;
  impulse.timestamp_us = 250000U;
  impulse.accel_g = {1.2F, 1.0F, 0.0F};
  impulse.gyro_dps = {0.0F, 0.0F, 360.0F};
  interpreter.update(impulse, identity);
  const auto snapshot = interpreter.snapshot();

  TEST_ASSERT_TRUE(snapshot.shake_energy > 0.2F);
  TEST_ASSERT_TRUE(snapshot.tap_impulse > 0.0F);
  TEST_ASSERT_TRUE(snapshot.spin_rate > 0.9F);
  TEST_ASSERT_TRUE(snapshot.gravity.y > 0.7F);
  TEST_ASSERT_TRUE(snapshot.gravity.x < 0.3F);
}

void test_long_randomized_scene_replay_remains_bounded() {
  Model first(lava_water_config(0xDEADBEEFULL));
  Model second(lava_water_config(0xDEADBEEFULL));
  std::uint32_t state = 0xC001D00DU;

  for (std::uint32_t tick = 0; tick < 1200U; ++tick) {
    state = state * 1664525U + 1013904223U;
    InputFrame frame{};
    switch ((state >> 28U) & 3U) {
    case 0:
      frame = gravity_frame(1.0F, 0.0F);
      break;
    case 1:
      frame = gravity_frame(-1.0F, 0.0F);
      break;
    case 2:
      frame = gravity_frame(0.0F, 1.0F);
      break;
    default:
      frame = gravity_frame(0.0F, -1.0F);
      break;
    }
    frame.shake_energy = static_cast<float>((state >> 8U) & 0xFFU) / 255.0F;
    frame.motion_energy = static_cast<float>((state >> 16U) & 0xFFU) / 255.0F;
    frame.tap_impulse = (state & 0x1FU) == 0U ? 1.0F : 0.0F;
    frame.cap_combo_event = (state & 0xFFU) == 1U;
    frame.cap_combo = frame.cap_combo_event ? 1.0F : 0.0F;

    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
    TEST_ASSERT_TRUE(first.invariants_hold());
    TEST_ASSERT_TRUE(first.tick_work_stats().events.used <= first.tick_work_stats().events.limit);
    TEST_ASSERT_TRUE(first.tick_work_stats().reactions.used <=
                     first.tick_work_stats().reactions.limit);
    TEST_ASSERT_EQUAL_UINT16(0U, first.world().totals().invalid_cells);
  }
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_scene_initialization_is_sparse_and_deterministic);
  RUN_TEST(test_autonomous_contact_creates_persistent_crust_and_steam);
  RUN_TEST(test_tilt_materially_changes_flow_geometry);
  RUN_TEST(test_shake_fractures_crust_without_mass_loss);
  RUN_TEST(test_combo_uses_shared_reaction_path_for_bounded_burst);
  RUN_TEST(test_pinch_slider_spawns_secondary_water_near_selected_x);
  RUN_TEST(test_reset_restores_exact_fixed_seed_scene);
  RUN_TEST(test_fixed_seed_trace_replays_identically);
  RUN_TEST(test_renderer_keeps_lava_water_crust_and_steam_distinct);
  RUN_TEST(test_motion_interpreter_produces_stable_gravity_without_false_shake);
  RUN_TEST(test_motion_impulse_is_separate_from_low_pass_gravity);
  RUN_TEST(test_long_randomized_scene_replay_remains_bounded);
  return UNITY_END();
}
