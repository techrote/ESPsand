#include <unity.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/io/types.hpp>
#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/materials.hpp>
#include <espsand/sim/model.hpp>
#include <espsand/sim/scene_limits.hpp>
#include <espsand/sim/world.hpp>

namespace {

using espsand::io::Frame8x8;
using espsand::io::Rgb;
using espsand::render::WorldRenderer;
using espsand::sim::Cell;
using espsand::sim::InputFrame;
using espsand::sim::MaterialId;
using espsand::sim::Model;
using espsand::sim::ModelConfig;
using espsand::sim::SceneId;
using espsand::sim::World;

ModelConfig scene_config(SceneId scene, std::uint64_t seed = 0xB10C2026ULL) {
  ModelConfig config{};
  config.seed = seed;
  config.scene = scene;
  config.event_budget = 12;
  config.reaction_budget = 12;
  return config;
}

InputFrame downward_gravity() {
  InputFrame frame{};
  frame.gravity = {0.0F, 1.0F};
  frame.gravity_magnitude = 1.0F;
  frame.gravity_confidence = 1.0F;
  return frame;
}

bool equal_rgb(const Rgb& first, const Rgb& second) {
  return first.r == second.r && first.g == second.g && first.b == second.b;
}

bool black(const Rgb& pixel) {
  return pixel.r == 0U && pixel.g == 0U && pixel.b == 0U;
}

std::uint32_t energy(const Rgb& pixel) {
  return static_cast<std::uint32_t>(pixel.r) + pixel.g + pixel.b;
}

std::size_t unique_nonblack_colors(const Frame8x8& frame) {
  std::array<Rgb, espsand::io::kMatrixPixels> unique{};
  std::size_t count = 0;
  for (const Rgb& pixel : frame) {
    if (black(pixel)) {
      continue;
    }
    bool found = false;
    for (std::size_t index = 0; index < count; ++index) {
      if (equal_rgb(unique[index], pixel)) {
        found = true;
        break;
      }
    }
    if (!found) {
      unique[count++] = pixel;
    }
  }
  return count;
}

std::size_t longest_equal_nonblack_run(const Frame8x8& frame) {
  std::size_t longest = 0;
  for (std::size_t y = 0; y < espsand::io::kMatrixHeight; ++y) {
    std::size_t run = 0;
    Rgb previous{};
    bool have_previous = false;
    for (std::size_t x = 0; x < espsand::io::kMatrixWidth; ++x) {
      const Rgb pixel = frame[y * espsand::io::kMatrixWidth + x];
      if (!black(pixel) && have_previous && equal_rgb(pixel, previous)) {
        ++run;
      } else {
        run = black(pixel) ? 0U : 1U;
      }
      previous = pixel;
      have_previous = !black(pixel);
      longest = longest > run ? longest : run;
    }
  }

  for (std::size_t x = 0; x < espsand::io::kMatrixWidth; ++x) {
    std::size_t run = 0;
    Rgb previous{};
    bool have_previous = false;
    for (std::size_t y = 0; y < espsand::io::kMatrixHeight; ++y) {
      const Rgb pixel = frame[y * espsand::io::kMatrixWidth + x];
      if (!black(pixel) && have_previous && equal_rgb(pixel, previous)) {
        ++run;
      } else {
        run = black(pixel) ? 0U : 1U;
      }
      previous = pixel;
      have_previous = !black(pixel);
      longest = longest > run ? longest : run;
    }
  }
  return longest;
}

void assert_product_material_caps(const Model& model) {
  const auto totals = model.world().totals();
  for (std::size_t index = 1U; index < espsand::sim::kMaterialCount; ++index) {
    TEST_ASSERT_TRUE(totals.cell_count[index] <= espsand::sim::kProductMaterialCellLimit);
  }
}

Cell water_cell() {
  Cell cell{};
  cell.material = MaterialId::kWater;
  cell.mass = 220U;
  cell.temperature = 20;
  return cell;
}

void test_partial_logical_coverage_projects_dimmer_than_full_coverage() {
  World sparse;
  World full;
  const Cell water = water_cell();
  TEST_ASSERT_TRUE(sparse.set_cell(0, 0, water));
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      TEST_ASSERT_TRUE(full.set_cell(x, y, water));
    }
  }

  const Rgb sparse_pixel = WorldRenderer{}.render(sparse).frame[0];
  const Rgb full_pixel = WorldRenderer{}.render(full).frame[0];
  TEST_ASSERT_TRUE(energy(full_pixel) > energy(sparse_pixel));
}

void test_uniform_liquid_projection_has_deterministic_surface_structure() {
  World world;
  const Cell water = water_cell();
  for (int y = 0; y < static_cast<int>(espsand::sim::kWorldHeight); ++y) {
    for (int x = 0; x < static_cast<int>(espsand::sim::kWorldWidth); ++x) {
      TEST_ASSERT_TRUE(world.set_cell(x, y, water));
    }
  }

  const Frame8x8 first = WorldRenderer{}.render(world).frame;
  const Frame8x8 second = WorldRenderer{}.render(world).frame;
  for (std::size_t index = 0; index < first.size(); ++index) {
    TEST_ASSERT_TRUE(equal_rgb(first[index], second[index]));
  }
  TEST_ASSERT_TRUE(unique_nonblack_colors(first) >= 4U);
  TEST_ASSERT_TRUE(longest_equal_nonblack_run(first) <= 4U);
}

void test_product_initial_frames_are_sparse_and_structured() {
  constexpr std::array<SceneId, 4> kScenes{{
      SceneId::kLavaWater,
      SceneId::kSodiumWater,
      SceneId::kOilFire,
      SceneId::kMossGarden,
  }};

  for (SceneId scene : kScenes) {
    const Model model(scene_config(scene));
    assert_product_material_caps(model);
    const Frame8x8 frame = WorldRenderer{}.render(model.world()).frame;
    TEST_ASSERT_TRUE(unique_nonblack_colors(frame) >= 4U);
    TEST_ASSERT_TRUE(longest_equal_nonblack_run(frame) <= 4U);
  }
}

void test_product_materials_remain_below_sixteen_cells_during_long_resting_run() {
  constexpr std::array<SceneId, 4> kScenes{{
      SceneId::kLavaWater,
      SceneId::kSodiumWater,
      SceneId::kOilFire,
      SceneId::kMossGarden,
  }};

  for (SceneId scene : kScenes) {
    Model model(scene_config(scene, 0x51A0D900ULL + static_cast<std::uint8_t>(scene)));
    assert_product_material_caps(model);
    for (std::uint32_t tick = 0; tick < 720U; ++tick) {
      model.step(downward_gravity());
      assert_product_material_caps(model);
      TEST_ASSERT_TRUE(model.invariants_hold());
    }
  }
}

void test_product_frames_remain_structured_after_settling() {
  constexpr std::array<SceneId, 4> kScenes{{
      SceneId::kLavaWater,
      SceneId::kSodiumWater,
      SceneId::kOilFire,
      SceneId::kMossGarden,
  }};

  for (SceneId scene : kScenes) {
    Model model(scene_config(scene, 0x51A0DA00ULL + static_cast<std::uint8_t>(scene)));
    for (std::uint32_t tick = 0; tick < 180U; ++tick) {
      model.step(downward_gravity());
    }
    const Frame8x8 frame = WorldRenderer{}.render(model.world()).frame;
    TEST_ASSERT_TRUE(unique_nonblack_colors(frame) >= 3U);
    TEST_ASSERT_TRUE(longest_equal_nonblack_run(frame) <= 5U);
    assert_product_material_caps(model);
    TEST_ASSERT_TRUE(model.invariants_hold());
  }
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_partial_logical_coverage_projects_dimmer_than_full_coverage);
  RUN_TEST(test_uniform_liquid_projection_has_deterministic_surface_structure);
  RUN_TEST(test_product_initial_frames_are_sparse_and_structured);
  RUN_TEST(test_product_materials_remain_below_sixteen_cells_during_long_resting_run);
  RUN_TEST(test_product_frames_remain_structured_after_settling);
  return UNITY_END();
}
