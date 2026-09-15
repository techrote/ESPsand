#include <unity.h>

#include <cmath>
#include <cstdint>

#include <espsand/input/motion_interpreter.hpp>
#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/model.hpp>
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

void test_scene_initialization_is_strong_and_deterministic() {
  Model first(lava_water_config());
  Model second(lava_water_config());

  TEST_ASSERT_TRUE(first.invariants_hold());
  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  TEST_ASSERT_TRUE(material_mass(first, MaterialId::kWater) >= 12000U);
  TEST_ASSERT_TRUE(material_mass(first, MaterialId::kLava) >= 2500U);
  TEST_ASSERT_EQUAL_UINT32(0U, material_mass(first, MaterialId::kSteam));
}

void test_autonomous_contact_creates_persistent_crust_and_steam() {
  Model model(lava_water_config());
  const std::uint32_t initial_mass = model.world().totals().total_mass;

  for (std::uint32_t tick = 0; tick < 18U; ++tick) {
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
  for (std::uint32_t tick = 0; tick < 12U; ++tick) {
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
  Model model(lava_water_config(321));
  InputFrame frame{};
  frame.cap_combo = 1.0F;
  frame.cap_combo_event = true;
  model.step(frame);

  TEST_ASSERT_EQUAL_UINT16(1U, model.lava_water_stats().burst_pairs);
  TEST_ASSERT_TRUE(model.dynamics_stats().reactions_applied >= 1U);
  const auto work = model.tick_work_stats();
  TEST_ASSERT_TRUE(work.events.used <= work.events.limit);
  TEST_ASSERT_TRUE(work.reactions.used <= work.reactions.limit);
}

void test_pinch_slider_injects_lava_at_bounded_position() {
  Model model(lava_water_config(777));
  InputFrame frame{};
  frame.slider_active = true;
  frame.slider_position = 0.75F;
  frame.slider_strength = 0.9F;
  model.step(frame);

  const auto stats = model.lava_water_stats();
  TEST_ASSERT_EQUAL_UINT16(1U, stats.touch_lava_injections);
  TEST_ASSERT_TRUE(stats.last_injection_x >= 9U);
  TEST_ASSERT_TRUE(stats.last_injection_x <= 14U);
}

void test_reset_restores_exact_fixed_seed_scene() {
  Model model(lava_water_config(0x12345678ULL));
  const std::uint64_t initial_hash = model.state_hash();

  for (std::uint32_t tick = 0; tick < 80U; ++tick) {
    model.step(trace_frame(tick));
  }
  TEST_ASSERT_TRUE(model.state_hash() != initial_hash);

  model.reset();
  TEST_ASSERT_EQUAL_UINT64(initial_hash, model.state_hash());
  TEST_ASSERT_EQUAL_UINT64(0U, model.tick());
}

void test_fixed_seed_trace_replays_identically() {
  Model first(lava_water_config(0x0BADC0DEULL));
  Model second(lava_water_config(0x0BADC0DEULL));

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
  fill_block(world, 0, 0, material_cell(MaterialId::kWater, 255, 20));
  fill_block(world, 2, 0, material_cell(MaterialId::kLava, 255, 1600));
  fill_block(world, 4, 0, material_cell(MaterialId::kCrust, 255, 100));
  fill_block(world, 6, 0, material_cell(MaterialId::kSteam, 200, 700));

  const auto frame = WorldRenderer{}.render(world).frame;
  const std::uint16_t lava_energy =
      static_cast<std::uint16_t>(frame[1].r + frame[1].g + frame[1].b);
  const std::uint16_t crust_energy =
      static_cast<std::uint16_t>(frame[2].r + frame[2].g + frame[2].b);

  TEST_ASSERT_TRUE(frame[0].b > frame[0].r);
  TEST_ASSERT_TRUE(frame[1].r > frame[1].b);
  TEST_ASSERT_TRUE(crust_energy < lava_energy);
  TEST_ASSERT_TRUE(frame[3].b > frame[3].r);
}

void test_motion_interpreter_produces_stable_gravity_without_false_shake() {
  MotionInterpreter interpreter;
  PlaneTransform transform{};

  for (std::uint64_t sample_index = 0; sample_index < 20U; ++sample_index) {
    ImuSample sample{};
    sample.timestamp_us = 1000U + sample_index * 5000U;
    sample.accel_g = {0.0F, 1.0F, 0.0F};
    sample.valid = true;
    interpreter.update(sample, transform);
  }

  const auto snapshot = interpreter.snapshot();
  TEST_ASSERT_TRUE(snapshot.ready);
  TEST_ASSERT_TRUE(snapshot.gravity.y > 0.95F);
  TEST_ASSERT_TRUE(std::fabs(snapshot.gravity.x) < 0.05F);
  TEST_ASSERT_TRUE(snapshot.gravity_confidence > 0.95F);
  TEST_ASSERT_TRUE(snapshot.shake_energy < 0.05F);
  TEST_ASSERT_TRUE(snapshot.motion_energy < 0.05F);
}

void test_motion_impulse_is_separate_from_low_pass_gravity() {
  MotionInterpreter interpreter;
  PlaneTransform transform{};

  ImuSample calm{};
  calm.timestamp_us = 1000U;
  calm.accel_g = {0.0F, 1.0F, 0.0F};
  calm.valid = true;
  interpreter.update(calm, transform);

  ImuSample impulse = calm;
  impulse.timestamp_us = 200000U;
  impulse.accel_g = {1.2F, 1.0F, 0.0F};
  impulse.gyro_dps = {0.0F, 0.0F, 180.0F};
  interpreter.update(impulse, transform);

  const auto snapshot = interpreter.snapshot();
  TEST_ASSERT_TRUE(snapshot.shake_energy > 0.5F);
  TEST_ASSERT_TRUE(snapshot.tap_impulse > 0.3F);
  TEST_ASSERT_TRUE(snapshot.motion_energy > 0.5F);
  TEST_ASSERT_TRUE(snapshot.gravity.y > std::fabs(snapshot.gravity.x));
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 0.5F, snapshot.spin_rate);

  interpreter.consume_transients();
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, interpreter.snapshot().tap_impulse);
}

void test_long_randomized_scene_replay_remains_bounded() {
  Model first(lava_water_config(0x5555AAAAULL));
  Model second(lava_water_config(0x5555AAAAULL));
  std::uint32_t state = 0xC001CAFEU;

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
    frame.cap_combo_event = (state & 0x7FU) == 1U;
    frame.cap_combo = frame.cap_combo_event ? 1.0F : 0.0F;

    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
    TEST_ASSERT_TRUE(first.invariants_hold());
    const auto work = first.tick_work_stats();
    TEST_ASSERT_TRUE(work.events.used <= work.events.limit);
    TEST_ASSERT_TRUE(work.reactions.used <= work.reactions.limit);
    TEST_ASSERT_EQUAL_UINT16(0U, first.world().totals().invalid_cells);
  }
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_scene_initialization_is_strong_and_deterministic);
  RUN_TEST(test_autonomous_contact_creates_persistent_crust_and_steam);
  RUN_TEST(test_tilt_materially_changes_flow_geometry);
  RUN_TEST(test_shake_fractures_crust_without_mass_loss);
  RUN_TEST(test_combo_uses_shared_reaction_path_for_bounded_burst);
  RUN_TEST(test_pinch_slider_injects_lava_at_bounded_position);
  RUN_TEST(test_reset_restores_exact_fixed_seed_scene);
  RUN_TEST(test_fixed_seed_trace_replays_identically);
  RUN_TEST(test_renderer_keeps_lava_water_crust_and_steam_distinct);
  RUN_TEST(test_motion_interpreter_produces_stable_gravity_without_false_shake);
  RUN_TEST(test_motion_impulse_is_separate_from_low_pass_gravity);
  RUN_TEST(test_long_randomized_scene_replay_remains_bounded);
  return UNITY_END();
}
