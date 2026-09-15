#include <unity.h>

#include <cmath>
#include <cstdint>

#include <espsand/render/scene_effects.hpp>
#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/model.hpp>
#include <espsand/sim/moss_garden_scene.hpp>
#include <espsand/sim/scene_limits.hpp>

namespace {

using espsand::render::WorldRenderer;
using espsand::sim::InputFrame;
using espsand::sim::MaterialId;
using espsand::sim::MiteState;
using espsand::sim::Model;
using espsand::sim::ModelConfig;
using espsand::sim::MossGardenScene;
using espsand::sim::MossGardenTickContext;
using espsand::sim::Pcg32;
using espsand::sim::SceneId;
using espsand::sim::WorkBudget;
using espsand::sim::World;

ModelConfig scene_config(SceneId scene, std::uint64_t seed = 0xE5009009ULL) {
  ModelConfig config{};
  config.seed = seed;
  config.scene = scene;
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

std::uint16_t material_cells(const World& world, MaterialId material) {
  return world.totals().cell_count[espsand::sim::material_index(material)];
}

bool mite_moved(const MiteState& before, const MiteState& after) {
  return before.x != after.x || before.y != after.y;
}

struct Centroid {
  float x = 0.0F;
  float y = 0.0F;
  std::uint32_t count = 0;
};

Centroid centroid_for(const Model& model, MaterialId material) {
  Centroid result{};
  for (std::size_t index = 0; index < espsand::sim::kWorldCapacity; ++index) {
    const auto* cell = model.world().try_cell_index(index);
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

void clear_material(World& world, MaterialId material) {
  for (std::size_t index = 0; index < espsand::sim::kWorldCapacity; ++index) {
    auto* cell = world.try_cell_index(index);
    if (cell != nullptr && cell->material == material) {
      *cell = {};
    }
  }
}

void test_product_scenes_do_not_reserve_a_wall_border() {
  for (SceneId scene : espsand::sim::kProductSceneOrder) {
    Model model(scene_config(scene));
    TEST_ASSERT_EQUAL_UINT32(0U, material_mass(model, MaterialId::kWall));
  }
}

void test_moss_scene_initialization_is_deterministic_sparse_and_alive() {
  Model first(scene_config(SceneId::kMossGarden));
  Model second(scene_config(SceneId::kMossGarden));

  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  TEST_ASSERT_TRUE(first.invariants_hold());
  TEST_ASSERT_TRUE(material_mass(first, MaterialId::kWater) > 0U);
  TEST_ASSERT_TRUE(material_mass(first, MaterialId::kMoss) > 0U);
  TEST_ASSERT_TRUE(material_cells(first.world(), MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_TRUE(material_cells(first.world(), MaterialId::kMoss) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_EQUAL_UINT8(1U, first.moss_garden_state().mite_count);
}

void test_growth_requires_moisture() {
  World world;
  Pcg32 prng(1234U);
  MossGardenScene scene;
  scene.initialize(world, prng);
  clear_material(world, MaterialId::kWater);
  WorkBudget budget(12);
  const auto stats = scene.before_dynamics(
      MossGardenTickContext{world, prng, gravity_frame(0.0F, 1.0F), 0U, budget});

  TEST_ASSERT_EQUAL_UINT16(0U, stats.growth_cells);
  TEST_ASSERT_EQUAL_UINT16(0U, stats.reinforced_cells);
}

void test_wet_habitat_generates_bounded_growth() {
  World world;
  Pcg32 prng(5678U);
  MossGardenScene scene;
  scene.initialize(world, prng);
  WorkBudget budget(12);
  const auto stats = scene.before_dynamics(
      MossGardenTickContext{world, prng, gravity_frame(0.0F, 1.0F), 0U, budget});

  TEST_ASSERT_TRUE(stats.growth_cells + stats.reinforced_cells > 0U);
  TEST_ASSERT_TRUE(scene.snapshot().growth_energy <= 512U);
  TEST_ASSERT_TRUE(material_cells(world, MaterialId::kMoss) <=
                   espsand::sim::kProductMaterialCellLimit);
}

void test_mites_feed_and_gain_energy_from_biomass() {
  World world;
  Pcg32 prng(42U);
  MossGardenScene scene;
  scene.initialize(world, prng);
  const auto before = scene.snapshot();
  WorkBudget budget(12);
  const auto stats = scene.before_dynamics(
      MossGardenTickContext{world, prng, gravity_frame(0.0F, 1.0F), 0U, budget});
  const auto after = scene.snapshot();

  TEST_ASSERT_TRUE(stats.feeds > 0U);
  TEST_ASSERT_TRUE(after.mites[0].energy > before.mites[0].energy);
  TEST_ASSERT_FALSE(after.mites[1].active);
}

void test_mites_starve_without_biomass_and_remain_bounded() {
  World world;
  Pcg32 prng(77U);
  MossGardenScene scene;
  scene.initialize(world, prng);
  clear_material(world, MaterialId::kMoss);
  WorkBudget budget(12);
  const InputFrame gravity = gravity_frame(0.0F, 1.0F);

  for (std::uint64_t tick = 0; tick < 360U; ++tick) {
    budget.reset(12);
    scene.before_dynamics(MossGardenTickContext{world, prng, gravity, tick, budget});
    TEST_ASSERT_TRUE(scene.invariants_hold(world));
  }

  TEST_ASSERT_EQUAL_UINT8(0U, scene.snapshot().mite_count);
}

void test_fixed_seed_replays_mite_paths_and_ecology() {
  Model first(scene_config(SceneId::kMossGarden, 0x51504D05ULL));
  Model second(scene_config(SceneId::kMossGarden, 0x51504D05ULL));

  for (std::uint32_t tick = 0; tick < 500U; ++tick) {
    InputFrame frame{};
    switch ((tick / 50U) % 4U) {
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
    if (tick == 120U || tick == 330U) {
      frame.shake_energy = 1.0F;
    }
    if (tick == 200U) {
      frame.cap_combo = 1.0F;
      frame.cap_combo_event = true;
    }
    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
    TEST_ASSERT_TRUE(first.invariants_hold());
  }
}

void test_tilt_changes_future_growth_geometry() {
  Model downward(scene_config(SceneId::kMossGarden, 123U));
  Model upward(scene_config(SceneId::kMossGarden, 123U));

  for (std::uint32_t tick = 0; tick < 240U; ++tick) {
    downward.step(gravity_frame(0.0F, 1.0F));
    upward.step(gravity_frame(0.0F, -1.0F));
  }

  const Centroid down_moss = centroid_for(downward, MaterialId::kMoss);
  const Centroid up_moss = centroid_for(upward, MaterialId::kMoss);
  TEST_ASSERT_TRUE(down_moss.count != 0U);
  TEST_ASSERT_TRUE(up_moss.count != 0U);
  TEST_ASSERT_TRUE(std::fabs(down_moss.y - up_moss.y) > 0.25F ||
                   material_mass(downward, MaterialId::kMoss) !=
                       material_mass(upward, MaterialId::kMoss));
  TEST_ASSERT_TRUE(downward.state_hash() != upward.state_hash());
}

void test_mite_moves_independently_of_world_cells() {
  Model model(scene_config(SceneId::kMossGarden, 456U));
  const MiteState start = model.moss_garden_state().mites[0];
  bool saw_move = false;
  for (std::uint32_t tick = 0; tick < 40U; ++tick) {
    model.step(gravity_frame(0.0F, 1.0F));
    saw_move = saw_move || mite_moved(start, model.moss_garden_state().mites[0]);
  }
  TEST_ASSERT_TRUE(saw_move);
}

void test_mite_overlay_survives_downsampling() {
  Model model(scene_config(SceneId::kMossGarden, 789U));
  auto frame = WorldRenderer{}.render(model.world()).frame;
  const auto state = model.moss_garden_state();
  const auto& mite = state.mites[0];
  const std::size_t output_index = (mite.y / 2U) * espsand::io::kMatrixWidth + mite.x / 2U;
  const auto before = frame[output_index];
  espsand::render::apply_mite_overlay(frame, state);
  const auto after = frame[output_index];

  TEST_ASSERT_TRUE(after.b >= 245U);
  TEST_ASSERT_TRUE(after.r != before.r || after.g != before.g || after.b != before.b);
}

void test_temporal_persistence_is_bounded_and_deterministic() {
  espsand::io::Frame8x8 first{};
  espsand::io::Frame8x8 second{};
  for (auto& pixel : second) {
    pixel = {255, 128, 64};
  }
  auto a = first;
  auto b = first;
  espsand::render::blend_with_previous(a, second, 64U);
  espsand::render::blend_with_previous(b, second, 64U);

  TEST_ASSERT_EQUAL_UINT8(a[0].r, b[0].r);
  TEST_ASSERT_TRUE(a[0].r > 0U && a[0].r < 255U);
  TEST_ASSERT_TRUE(a[0].g < a[0].r);
}

void test_slider_spawns_water_near_selected_x_and_shake_scatter_is_bounded() {
  Model touched(scene_config(SceneId::kMossGarden, 321U));
  const std::uint16_t water_before = material_cells(touched.world(), MaterialId::kWater);
  InputFrame touch_frame{};
  touch_frame.slider_active = true;
  touch_frame.slider_position = 0.8F;
  touch_frame.slider_strength = 1.0F;
  touched.step(touch_frame);

  TEST_ASSERT_EQUAL_UINT8(1U, touched.moss_garden_state().mite_count);
  TEST_ASSERT_EQUAL_UINT16(1U, touched.moss_garden_stats().rain_pulses);
  TEST_ASSERT_TRUE(material_cells(touched.world(), MaterialId::kWater) > water_before);
  TEST_ASSERT_TRUE(material_cells(touched.world(), MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);

  bool found_near_touch = false;
  for (int y = 0; y <= 1; ++y) {
    for (int x = 9; x <= 15; ++x) {
      const auto* cell = touched.world().try_cell(x, y);
      if (cell != nullptr && cell->material == MaterialId::kWater && cell->mass != 0U) {
        found_near_touch = true;
      }
    }
  }
  TEST_ASSERT_TRUE(found_near_touch);
  TEST_ASSERT_TRUE(touched.tick_work_stats().events.used <= touched.tick_work_stats().events.limit);

  Model shaken(scene_config(SceneId::kMossGarden, 654U));
  InputFrame shake = gravity_frame(0.0F, 1.0F);
  shake.shake_energy = 1.0F;
  shaken.step(shake);
  TEST_ASSERT_EQUAL_UINT16(1U, shaken.moss_garden_stats().scatter_events);
  TEST_ASSERT_TRUE(shaken.tick_work_stats().events.used <= shaken.tick_work_stats().events.limit);
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_product_scenes_do_not_reserve_a_wall_border);
  RUN_TEST(test_moss_scene_initialization_is_deterministic_sparse_and_alive);
  RUN_TEST(test_growth_requires_moisture);
  RUN_TEST(test_wet_habitat_generates_bounded_growth);
  RUN_TEST(test_mites_feed_and_gain_energy_from_biomass);
  RUN_TEST(test_mites_starve_without_biomass_and_remain_bounded);
  RUN_TEST(test_fixed_seed_replays_mite_paths_and_ecology);
  RUN_TEST(test_tilt_changes_future_growth_geometry);
  RUN_TEST(test_mite_moves_independently_of_world_cells);
  RUN_TEST(test_mite_overlay_survives_downsampling);
  RUN_TEST(test_temporal_persistence_is_bounded_and_deterministic);
  RUN_TEST(test_slider_spawns_water_near_selected_x_and_shake_scatter_is_bounded);
  return UNITY_END();
}
