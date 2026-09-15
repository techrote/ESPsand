#include <espsand/sim/lava_water_scene.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace espsand::sim {
namespace {

constexpr std::uint64_t kAutoLavaPeriodTicks = 48;
constexpr std::uint64_t kAutoWaterPeriodTicks = 120;
constexpr std::uint64_t kTouchLavaPeriodTicks = 12;
constexpr std::uint8_t kLavaMass = 232;
constexpr std::uint8_t kWaterMass = 220;
constexpr std::int16_t kLavaTemperature = 1700;
constexpr std::int16_t kWaterTemperature = 24;

Cell material_cell(MaterialId material, std::uint8_t mass, std::int16_t temperature = 0) noexcept {
  Cell cell{};
  cell.material = material;
  cell.mass = mass;
  cell.temperature = temperature;
  return cell;
}

void add_border(World& world) noexcept {
  const Cell wall = material_cell(MaterialId::kWall, 255);
  for (std::size_t y = 0; y < kWorldHeight; ++y) {
    for (std::size_t x = 0; x < kWorldWidth; ++x) {
      if (x == 0U || y == 0U || x + 1U == kWorldWidth || y + 1U == kWorldHeight) {
        static_cast<void>(world.set_cell(static_cast<int>(x), static_cast<int>(y), wall));
      }
    }
  }
}

bool can_inject_into(const Cell& cell) noexcept {
  return cell.material == MaterialId::kEmpty || cell.material == MaterialId::kSteam ||
         cell.material == MaterialId::kSmoke || cell.material == MaterialId::kFire;
}

bool try_inject(World& world, int x, int y, const Cell& material) noexcept {
  Cell* target = world.try_cell(x, y);
  if (target == nullptr || !can_inject_into(*target)) {
    return false;
  }
  *target = material;
  return true;
}

std::uint8_t slider_to_interior_x(float position) noexcept {
  const float scaled = position * static_cast<float>(kWorldWidth - 3U);
  const auto rounded = static_cast<std::uint32_t>(scaled + 0.5F);
  return static_cast<std::uint8_t>(1U + std::min<std::uint32_t>(rounded, kWorldWidth - 3U));
}

bool inject_top(World& world, Pcg32& prng, MaterialId material, std::uint8_t mass,
                std::int16_t temperature, std::uint8_t& injected_x) noexcept {
  const std::uint32_t interior_width = static_cast<std::uint32_t>(kWorldWidth - 2U);
  const std::uint32_t start = prng.bounded(interior_width);
  for (std::uint32_t offset = 0; offset < interior_width; ++offset) {
    const std::uint32_t interior = (start + offset) % interior_width;
    const std::uint8_t x = static_cast<std::uint8_t>(1U + interior);
    if (try_inject(world, x, 1, material_cell(material, mass, temperature))) {
      injected_x = x;
      return true;
    }
  }
  return false;
}

bool inject_touch_lava(World& world, float position, std::uint8_t& injected_x) noexcept {
  const std::uint8_t preferred = slider_to_interior_x(position);
  constexpr std::array<int, 5> kOffsets{{0, -1, 1, -2, 2}};
  for (int offset : kOffsets) {
    const int x = static_cast<int>(preferred) + offset;
    if (x <= 0 || x + 1 >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    if (try_inject(world, x, 1,
                   material_cell(MaterialId::kLava, kLavaMass, kLavaTemperature))) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool inject_reaction_pair(World& world, Pcg32& prng) noexcept {
  const std::uint32_t interior_width = static_cast<std::uint32_t>(kWorldWidth - 3U);
  const std::uint32_t interior_height = static_cast<std::uint32_t>(kWorldHeight - 4U);
  const std::uint32_t candidates = interior_width * interior_height;
  const std::uint32_t start = prng.bounded(candidates);

  for (std::uint32_t offset = 0; offset < candidates; ++offset) {
    const std::uint32_t candidate = (start + offset) % candidates;
    const int x = 1 + static_cast<int>(candidate % interior_width);
    const int y = 2 + static_cast<int>(candidate / interior_width);
    Cell* first = world.try_cell(x, y);
    Cell* second = world.try_cell(x + 1, y);
    if (first == nullptr || second == nullptr || !can_inject_into(*first) ||
        !can_inject_into(*second)) {
      continue;
    }

    *first = material_cell(MaterialId::kLava, 210, 1800);
    *second = material_cell(MaterialId::kWater, 210, 20);
    return true;
  }
  return false;
}

bool fracture_one_crust(World& world, Pcg32& prng) noexcept {
  constexpr std::array<std::array<int, 2>, 4> kDirections{{
      {{1, 0}},
      {{-1, 0}},
      {{0, 1}},
      {{0, -1}},
  }};

  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldCapacity));
  for (std::uint32_t offset = 0; offset < kWorldCapacity; ++offset) {
    const std::size_t index = (start + offset) % kWorldCapacity;
    Cell* crust = world.try_cell_index(index);
    if (crust == nullptr || crust->material != MaterialId::kCrust) {
      continue;
    }

    const int x = static_cast<int>(index % kWorldWidth);
    const int y = static_cast<int>(index / kWorldWidth);
    const std::uint32_t direction_start = prng.bounded(kDirections.size());
    for (std::uint32_t direction_offset = 0; direction_offset < kDirections.size();
         ++direction_offset) {
      const auto& direction =
          kDirections[(direction_start + direction_offset) % kDirections.size()];
      const int target_x = x + direction[0];
      const int target_y = y + direction[1];
      Cell* target = world.try_cell(target_x, target_y);
      if (target == nullptr || target->material == MaterialId::kWall ||
          target->material == MaterialId::kCrust || target->material == MaterialId::kMoss) {
        continue;
      }

      const Cell displaced = *target;
      *target = *crust;
      *crust = displaced;
      return true;
    }
  }
  return false;
}

float disturbance_strength(const InputFrame& input) noexcept {
  return std::max({input.shake_energy, input.motion_energy, input.tap_impulse,
                   input.noise_event ? input.noise_impulse : 0.0F});
}

} // namespace

void LavaWaterScene::initialize(World& world, Pcg32& prng) const noexcept {
  world.clear();
  add_border(world);

  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  for (int y = 10; y <= 14; ++y) {
    for (int x = 2; x <= 13; ++x) {
      static_cast<void>(world.set_cell(x, y, water));
    }
  }

  const Cell lava = material_cell(MaterialId::kLava, kLavaMass, kLavaTemperature);
  for (int y = 3; y <= 4; ++y) {
    for (int x = 5; x <= 10; ++x) {
      static_cast<void>(world.set_cell(x, y, lava));
    }
  }

  const std::uint8_t contact_x = static_cast<std::uint8_t>(5U + prng.bounded(6U));
  static_cast<void>(world.set_cell(contact_x, 9, lava));
}

LavaWaterSceneStats LavaWaterScene::before_dynamics(World& world, Pcg32& prng,
                                                    const InputFrame& input,
                                                    std::uint64_t tick,
                                                    WorkBudget& event_budget) const noexcept {
  LavaWaterSceneStats stats{};

  if (tick != 0U && tick % kAutoLavaPeriodTicks == 0U) {
    if (inject_top(world, prng, MaterialId::kLava, kLavaMass, kLavaTemperature,
                   stats.last_injection_x)) {
      ++stats.autonomous_lava_injections;
    }
  }

  if (tick != 0U && tick % kAutoWaterPeriodTicks == 0U) {
    if (inject_top(world, prng, MaterialId::kWater, kWaterMass, kWaterTemperature,
                   stats.last_injection_x)) {
      ++stats.autonomous_water_injections;
    }
  }

  if (input.slider_active && input.slider_strength >= 0.35F &&
      tick % kTouchLavaPeriodTicks == 0U && event_budget.try_consume()) {
    if (inject_touch_lava(world, input.slider_position, stats.last_injection_x)) {
      ++stats.touch_lava_injections;
    }
  }

  if (input.cap_combo_event && event_budget.try_consume()) {
    if (inject_reaction_pair(world, prng)) {
      ++stats.burst_pairs;
    }
  }

  const float disturbance = disturbance_strength(input);
  if (disturbance >= 0.55F) {
    const std::uint16_t fracture_target =
        static_cast<std::uint16_t>(1U + (disturbance >= 0.85F ? 2U : 1U));
    for (std::uint16_t index = 0; index < fracture_target; ++index) {
      if (!event_budget.try_consume() || !fracture_one_crust(world, prng)) {
        break;
      }
      ++stats.crust_fractures;
    }
  }

  return stats;
}

} // namespace espsand::sim
