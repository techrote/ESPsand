#include <unity.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/model.hpp>
#include <espsand/sim/scene_limits.hpp>

namespace {

using espsand::render::WorldRenderer;
using espsand::sim::Cell;
using espsand::sim::InputFrame;
using espsand::sim::MaterialId;
using espsand::sim::Model;
using espsand::sim::ModelConfig;
using espsand::sim::SceneId;

ModelConfig scene_config(SceneId scene, std::uint64_t seed = 0xE5008008ULL) {
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

InputFrame trace_frame(std::uint32_t tick) {
  InputFrame frame{};
  switch ((tick / 20U) % 4U) {
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
  if (tick == 17U || tick == 73U) {
    frame.shake_energy = 1.0F;
  }
  if (tick == 33U) {
    frame.tap_impulse = 1.0F;
  }
  if (tick == 50U) {
    frame.cap_combo = 1.0F;
    frame.cap_combo_event = true;
  }
  if (tick >= 60U && tick < 64U) {
    frame.slider_active = true;
    frame.slider_position = 0.7F;
    frame.slider_strength = 0.9F;
  }
  return frame;
}

void test_product_scene_catalogue_is_stable() {
  const SceneId after_lava = espsand::sim::next_product_scene(SceneId::kLavaWater);
  const SceneId after_sodium = espsand::sim::next_product_scene(SceneId::kSodiumWater);
  const SceneId after_oil = espsand::sim::next_product_scene(SceneId::kOilFire);
  const SceneId after_moss = espsand::sim::next_product_scene(SceneId::kMossGarden);

  TEST_ASSERT_EQUAL_UINT8(4U, espsand::sim::kProductSceneOrder.size());
  TEST_ASSERT_TRUE(after_lava == SceneId::kSodiumWater);
  TEST_ASSERT_TRUE(after_sodium == SceneId::kOilFire);
  TEST_ASSERT_TRUE(after_oil == SceneId::kMossGarden);
  TEST_ASSERT_TRUE(after_moss == SceneId::kLavaWater);
  TEST_ASSERT_EQUAL_STRING("sodium_water", espsand::sim::scene_name(SceneId::kSodiumWater));
  TEST_ASSERT_EQUAL_STRING("oil_fire", espsand::sim::scene_name(SceneId::kOilFire));
  TEST_ASSERT_EQUAL_STRING("moss_garden", espsand::sim::scene_name(SceneId::kMossGarden));
}

void test_sodium_scene_initializes_sparse_water_and_finite_reactant_deterministically() {
  Model first(scene_config(SceneId::kSodiumWater));
  Model second(scene_config(SceneId::kSodiumWater));

  TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  TEST_ASSERT_TRUE(first.invariants_hold());
  TEST_ASSERT_EQUAL_UINT16(12U, material_cells(first, MaterialId::kWater));
  TEST_ASSERT_EQUAL_UINT16(4U, material_cells(first, MaterialId::kSodiumLike));
  TEST_ASSERT_TRUE(material_cells(first, MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_TRUE(material_cells(first, MaterialId::kSodiumLike) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_EQUAL_UINT32(0U, material_mass(first, MaterialId::kFire));
  TEST_ASSERT_EQUAL_UINT32(0U, material_mass(first, MaterialId::kSteam));
}

void test_sodium_contact_is_finite_and_drives_shared_reaction_impulse() {
  Model model(scene_config(SceneId::kSodiumWater, 77));
  const std::uint32_t initial_mass = model.world().totals().total_mass;
  const std::uint32_t initial_sodium = material_mass(model, MaterialId::kSodiumLike);
  bool saw_reaction = false;
  bool saw_impulse = false;

  for (std::uint32_t tick = 0; tick < 140U; ++tick) {
    model.step(gravity_frame(0.0F, 1.0F));
    saw_reaction = saw_reaction || model.dynamics_stats().reactions_applied != 0U;
    saw_impulse = saw_impulse || model.dynamics_stats().reaction_impulses != 0U;
  }

  TEST_ASSERT_TRUE(saw_reaction);
  TEST_ASSERT_TRUE(saw_impulse);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kSodiumLike) < initial_sodium);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kSteam) > 0U ||
                   material_mass(model, MaterialId::kSmoke) > 0U);
  TEST_ASSERT_EQUAL_UINT32(initial_mass, model.world().totals().total_mass);
}

void test_sodium_combo_is_bounded_and_uses_shared_reaction_path() {
  Model model(scene_config(SceneId::kSodiumWater, 123));
  InputFrame frame = gravity_frame(0.0F, 1.0F);
  frame.cap_combo = 1.0F;
  frame.cap_combo_event = true;
  model.step(frame);

  TEST_ASSERT_EQUAL_UINT16(1U, model.sodium_water_stats().burst_pairs);
  TEST_ASSERT_TRUE(model.dynamics_stats().reactions_applied >= 1U);
  TEST_ASSERT_TRUE(model.dynamics_stats().reaction_impulses <=
                   model.dynamics_stats().reactions_applied);
  TEST_ASSERT_TRUE(model.tick_work_stats().events.used <= model.tick_work_stats().events.limit);
  TEST_ASSERT_TRUE(model.tick_work_stats().reactions.used <=
                   model.tick_work_stats().reactions.limit);
}

void test_sodium_slider_spawns_secondary_water_near_selected_x() {
  Model model(scene_config(SceneId::kSodiumWater, 456));
  InputFrame frame{};
  frame.slider_active = true;
  frame.slider_position = 0.78F;
  frame.slider_strength = 0.9F;
  model.step(frame);

  const auto stats = model.sodium_water_stats();
  TEST_ASSERT_EQUAL_UINT16(1U, stats.touch_water_injections);
  TEST_ASSERT_TRUE(stats.last_injection_x >= 9U);
  TEST_ASSERT_TRUE(stats.last_injection_x <= 14U);
  TEST_ASSERT_TRUE(has_material_in_top_column(model, MaterialId::kWater, stats.last_injection_x));
  TEST_ASSERT_TRUE(material_cells(model, MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);
}

void test_sodium_fixed_seed_trace_replays_identically() {
  Model first(scene_config(SceneId::kSodiumWater, 0x51A0D1ULL));
  Model second(scene_config(SceneId::kSodiumWater, 0x51A0D1ULL));

  for (std::uint32_t tick = 0; tick < 160U; ++tick) {
    const InputFrame frame = trace_frame(tick);
    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  }
}

void test_oil_scene_starts_as_sparse_low_density_fuel_over_water() {
  Model model(scene_config(SceneId::kOilFire));
  const Centroid oil = centroid_for(model, MaterialId::kOil);
  const Centroid water = centroid_for(model, MaterialId::kWater);

  TEST_ASSERT_TRUE(oil.count > 0U);
  TEST_ASSERT_TRUE(water.count > 0U);
  TEST_ASSERT_TRUE(oil.y < water.y);
  TEST_ASSERT_TRUE(material_cells(model, MaterialId::kOil) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_TRUE(material_cells(model, MaterialId::kWater) <=
                   espsand::sim::kProductMaterialCellLimit);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kFire) > 0U);
  TEST_ASSERT_TRUE(model.invariants_hold());
}

void test_oil_fire_consumes_finite_fuel_and_extinguishes_before_refill_phase() {
  Model model(scene_config(SceneId::kOilFire, 9001));
  const std::uint32_t initial_mass = model.world().totals().total_mass;
  const std::uint32_t initial_oil = material_mass(model, MaterialId::kOil);
  bool saw_reaction = false;
  bool saw_expiry = false;

  for (std::uint32_t tick = 0; tick < 260U; ++tick) {
    model.step(gravity_frame(0.0F, 1.0F));
    saw_reaction = saw_reaction || model.dynamics_stats().reactions_applied != 0U;
    saw_expiry = saw_expiry || model.dynamics_stats().fire_cells_expired != 0U;
  }

  TEST_ASSERT_TRUE(saw_reaction);
  TEST_ASSERT_TRUE(saw_expiry);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kOil) < initial_oil);
  TEST_ASSERT_EQUAL_UINT32(0U, material_mass(model, MaterialId::kFire));
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kSmoke) > 0U);
  TEST_ASSERT_EQUAL_UINT32(initial_mass, model.world().totals().total_mass);
}

void test_oil_combo_ignites_existing_fuel_with_event_budget() {
  Model model(scene_config(SceneId::kOilFire, 222));
  const std::uint32_t fire_before = material_mass(model, MaterialId::kFire);
  InputFrame frame{};
  frame.cap_combo = 1.0F;
  frame.cap_combo_event = true;
  model.step(frame);

  TEST_ASSERT_EQUAL_UINT16(1U, model.oil_fire_stats().combo_ignitions);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kFire) >= fire_before);
  TEST_ASSERT_TRUE(model.tick_work_stats().events.used <= model.tick_work_stats().events.limit);
}

void test_oil_slider_ignites_secondary_fire_near_selected_x() {
  Model model(scene_config(SceneId::kOilFire, 333));
  const std::uint32_t oil_before = material_mass(model, MaterialId::kOil);
  InputFrame frame{};
  frame.slider_active = true;
  frame.slider_position = 0.2F;
  frame.slider_strength = 0.9F;
  model.step(frame);

  const auto stats = model.oil_fire_stats();
  TEST_ASSERT_EQUAL_UINT16(1U, stats.touch_ignitions);
  TEST_ASSERT_TRUE(stats.last_injection_x <= 6U);
  TEST_ASSERT_TRUE(material_mass(model, MaterialId::kOil) < oil_before);
}

void test_oil_fixed_seed_trace_replays_identically() {
  Model first(scene_config(SceneId::kOilFire, 0x01F1AEULL));
  Model second(scene_config(SceneId::kOilFire, 0x01F1AEULL));

  for (std::uint32_t tick = 0; tick < 560U; ++tick) {
    const InputFrame frame = trace_frame(tick);
    first.step(frame);
    second.step(frame);
    TEST_ASSERT_EQUAL_UINT64(first.state_hash(), second.state_hash());
  }
}

void test_three_energetic_scenes_render_distinct_initial_frames() {
  Model lava(scene_config(SceneId::kLavaWater, 12));
  Model sodium(scene_config(SceneId::kSodiumWater, 12));
  Model oil(scene_config(SceneId::kOilFire, 12));
  const auto lava_frame = WorldRenderer{}.render(lava.world()).frame;
  const auto sodium_frame = WorldRenderer{}.render(sodium.world()).frame;
  const auto oil_frame = WorldRenderer{}.render(oil.world()).frame;

  TEST_ASSERT_TRUE(std::memcmp(lava_frame.data(), sodium_frame.data(), sizeof(lava_frame)) != 0);
  TEST_ASSERT_TRUE(std::memcmp(lava_frame.data(), oil_frame.data(), sizeof(lava_frame)) != 0);
  TEST_ASSERT_TRUE(std::memcmp(sodium_frame.data(), oil_frame.data(), sizeof(lava_frame)) != 0);
}

void test_energetic_scenes_randomized_replay_remains_bounded() {
  constexpr SceneId kScenes[] = {SceneId::kSodiumWater, SceneId::kOilFire};
  for (SceneId scene : kScenes) {
    Model first(scene_config(scene, 0xAA550000ULL + static_cast<std::uint8_t>(scene)));
    Model second(scene_config(scene, 0xAA550000ULL + static_cast<std::uint8_t>(scene)));
    std::uint32_t state = 0x51C0FFEEU + static_cast<std::uint8_t>(scene);

    for (std::uint32_t tick = 0; tick < 1000U; ++tick) {
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
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_product_scene_catalogue_is_stable);
  RUN_TEST(test_sodium_scene_initializes_sparse_water_and_finite_reactant_deterministically);
  RUN_TEST(test_sodium_contact_is_finite_and_drives_shared_reaction_impulse);
  RUN_TEST(test_sodium_combo_is_bounded_and_uses_shared_reaction_path);
  RUN_TEST(test_sodium_slider_spawns_secondary_water_near_selected_x);
  RUN_TEST(test_sodium_fixed_seed_trace_replays_identically);
  RUN_TEST(test_oil_scene_starts_as_sparse_low_density_fuel_over_water);
  RUN_TEST(test_oil_fire_consumes_finite_fuel_and_extinguishes_before_refill_phase);
  RUN_TEST(test_oil_combo_ignites_existing_fuel_with_event_budget);
  RUN_TEST(test_oil_slider_ignites_secondary_fire_near_selected_x);
  RUN_TEST(test_oil_fixed_seed_trace_replays_identically);
  RUN_TEST(test_three_energetic_scenes_render_distinct_initial_frames);
  RUN_TEST(test_energetic_scenes_randomized_replay_remains_bounded);
  return UNITY_END();
}
