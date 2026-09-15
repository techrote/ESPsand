#include <unity.h>

#include <cstdint>

#include <espsand/render/output_limiter.hpp>
#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/world.hpp>

namespace {

using espsand::io::Frame8x8;
using espsand::render::OutputLimiter;
using espsand::render::OutputPolicy;
using espsand::render::RenderConfig;
using espsand::render::RenderMode;
using espsand::render::WorldRenderer;
using espsand::sim::Cell;
using espsand::sim::MaterialId;
using espsand::sim::World;

void fill_block(World& world, int base_x, int base_y, const Cell& cell) {
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      TEST_ASSERT_TRUE(world.set_cell(base_x + x, base_y + y, cell));
    }
  }
}

void test_renderer_is_deterministic_for_fixed_world() {
  World world;
  Cell water{};
  water.material = MaterialId::kWater;
  water.mass = 180;
  fill_block(world, 0, 0, water);

  Cell lava{};
  lava.material = MaterialId::kLava;
  lava.mass = 220;
  lava.temperature = 900;
  fill_block(world, 2, 0, lava);

  WorldRenderer renderer;
  const auto first = renderer.render(world);
  const auto second = renderer.render(world);

  TEST_ASSERT_EQUAL_MEMORY(first.frame.data(), second.frame.data(), sizeof(first.frame));
  TEST_ASSERT_EQUAL_UINT16(first.stats.unknown_material_cells,
                           second.stats.unknown_material_cells);
  TEST_ASSERT_EQUAL_UINT16(first.stats.minority_preserved_pixels,
                           second.stats.minority_preserved_pixels);
}

void test_mixed_block_preserves_bright_minority() {
  World all_water;
  Cell water{};
  water.material = MaterialId::kWater;
  water.mass = 255;
  fill_block(all_water, 0, 0, water);

  World mixed = all_water;
  Cell fire{};
  fire.material = MaterialId::kFire;
  fire.mass = 40;
  fire.temperature = 800;
  TEST_ASSERT_TRUE(mixed.set_cell(1, 1, fire));

  WorldRenderer renderer;
  const auto water_result = renderer.render(all_water);
  const auto mixed_result = renderer.render(mixed);

  TEST_ASSERT_TRUE(mixed_result.frame[0].r > water_result.frame[0].r);
  TEST_ASSERT_TRUE(mixed_result.frame[0].g > water_result.frame[0].g);
  TEST_ASSERT_EQUAL_UINT16(1U, mixed_result.stats.minority_preserved_pixels);
}

void test_palette_mapping_keeps_materials_visually_distinct() {
  World world;
  Cell water{};
  water.material = MaterialId::kWater;
  water.mass = 255;
  fill_block(world, 0, 0, water);

  Cell lava{};
  lava.material = MaterialId::kLava;
  lava.mass = 255;
  fill_block(world, 2, 0, lava);

  Cell moss{};
  moss.material = MaterialId::kMoss;
  moss.mass = 255;
  fill_block(world, 4, 0, moss);

  const auto frame = WorldRenderer{}.render(world).frame;
  TEST_ASSERT_TRUE(frame[0].b > frame[0].r);
  TEST_ASSERT_TRUE(frame[1].r > frame[1].b);
  TEST_ASSERT_TRUE(frame[2].g > frame[2].r);
}

void test_render_diagnostic_modes_are_deterministic() {
  World world;
  Cell cell{};
  cell.material = MaterialId::kTracer;
  cell.mass = 192;
  cell.temperature = 1200;
  cell.aux = 200;
  fill_block(world, 0, 0, cell);

  WorldRenderer renderer;
  for (RenderMode mode : {RenderMode::kBeauty, RenderMode::kMaterialId,
                          RenderMode::kTemperature, RenderMode::kMass}) {
    RenderConfig config{};
    config.mode = mode;
    const auto first = renderer.render(world, config);
    const auto second = renderer.render(world, config);
    TEST_ASSERT_EQUAL_MEMORY(first.frame.data(), second.frame.data(), sizeof(first.frame));
    TEST_ASSERT_TRUE(first.frame[0].r != 0U || first.frame[0].g != 0U ||
                     first.frame[0].b != 0U);
  }
}

void test_unknown_material_is_safe_and_counted() {
  World world;
  Cell* cell = world.try_cell(0, 0);
  TEST_ASSERT_NOT_NULL(cell);
  cell->material = static_cast<MaterialId>(250U);
  cell->mass = 255;

  RenderConfig config{};
  config.mode = RenderMode::kMaterialId;
  const auto result = WorldRenderer{}.render(world, config);

  TEST_ASSERT_EQUAL_UINT16(1U, result.stats.unknown_material_cells);
  TEST_ASSERT_TRUE(result.frame[0].r > 0U);
  TEST_ASSERT_TRUE(result.frame[0].b > 0U);
}

void test_output_limiter_clamps_dense_white_below_ceiling() {
  Frame8x8 frame{};
  frame.fill({255, 255, 255});

  OutputLimiter limiter(OutputPolicy{32, 4096});
  const auto decision = limiter.limit(frame, 255);

  TEST_ASSERT_EQUAL_UINT8(21U, decision.applied_brightness);
  TEST_ASSERT_TRUE(decision.ceiling_limited);
  TEST_ASSERT_TRUE(decision.load_limited);
  TEST_ASSERT_TRUE(decision.estimated_frame_load <= 4096U);
}

void test_output_limiter_allows_sparse_frame_to_ceiling() {
  Frame8x8 frame{};
  frame[0] = {255, 255, 255};

  OutputLimiter limiter(OutputPolicy{32, 4096});
  const auto decision = limiter.limit(frame, 255);

  TEST_ASSERT_EQUAL_UINT8(32U, decision.applied_brightness);
  TEST_ASSERT_TRUE(decision.ceiling_limited);
  TEST_ASSERT_FALSE(decision.load_limited);
}

void test_zero_load_budget_fails_dark_for_nonempty_frame() {
  Frame8x8 frame{};
  frame[0] = {255, 0, 0};

  OutputLimiter limiter(OutputPolicy{32, 0});
  const auto decision = limiter.limit(frame, 32);

  TEST_ASSERT_EQUAL_UINT8(0U, decision.applied_brightness);
  TEST_ASSERT_TRUE(decision.load_limited);
  TEST_ASSERT_EQUAL_UINT32(0U, decision.estimated_frame_load);
}

void test_randomized_world_render_stays_repeatable() {
  std::uint32_t state = 0x12345678U;
  auto next = [&state]() {
    state = state * 1664525U + 1013904223U;
    return state;
  };

  World world;
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      Cell cell{};
      cell.material =
          static_cast<MaterialId>(next() % static_cast<std::uint32_t>(MaterialId::kCount));
      cell.mass = static_cast<std::uint8_t>(next() >> 24U);
      cell.temperature = static_cast<std::int16_t>(next() & 0x0FFFU);
      cell.aux = static_cast<std::uint8_t>(next() >> 24U);
      TEST_ASSERT_TRUE(world.set_cell(x, y, cell));
    }
  }

  WorldRenderer renderer;
  const auto first = renderer.render(world);
  const auto second = renderer.render(world);
  TEST_ASSERT_EQUAL_MEMORY(first.frame.data(), second.frame.data(), sizeof(first.frame));
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_renderer_is_deterministic_for_fixed_world);
  RUN_TEST(test_mixed_block_preserves_bright_minority);
  RUN_TEST(test_palette_mapping_keeps_materials_visually_distinct);
  RUN_TEST(test_render_diagnostic_modes_are_deterministic);
  RUN_TEST(test_unknown_material_is_safe_and_counted);
  RUN_TEST(test_output_limiter_clamps_dense_white_below_ceiling);
  RUN_TEST(test_output_limiter_allows_sparse_frame_to_ceiling);
  RUN_TEST(test_zero_load_budget_fails_dark_for_nonempty_frame);
  RUN_TEST(test_randomized_world_render_stays_repeatable);
  return UNITY_END();
}
