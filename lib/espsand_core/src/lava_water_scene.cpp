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

std::uint8_t slider_to_x(float position) noexcept {
  const float scaled = position * static_cast<float>(kWorldWidth - 1U);
  return static_cast<std::uint8_t>(
      std::min<std::uint32_t>(kWorldWidth - 1U, static_cast<std::uint32_t>(scaled + 0.5F)));
}

bool inject_lava_vent(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  constexpr std::array<int, 8> kVentXs{{7, 8, 6, 9, 5, 10, 4, 11}};
  const std::uint32_t start = prng.bounded(4U);
  const Cell lava = material_cell(MaterialId::kLava, kLavaMass, kLavaTemperature);
  for (std::uint32_t offset = 0; offset < kVentXs.size(); ++offset) {
    const int x = kVentXs[(start + offset) % kVentXs.size()];
    if (try_inject(world, x, 0, lava)) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool refill_water_basin(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldWidth));
  for (int y = static_cast<int>(kWorldHeight) - 1; y >= 10; --y) {
    for (std::uint32_t offset = 0; offset < kWorldWidth; ++offset) {
      const int x = static_cast<int>((start + offset) % kWorldWidth);
      if (try_inject(world, x, y, water)) {
        injected_x = static_cast<std::uint8_t>(x);
        return true;
      }
    }
  }
  return false;
}

bool inject_touch_lava(World& world, float position, std::uint8_t& injected_x) noexcept {
  const std::uint8_t preferred = slider_to_x(position);
  constexpr std::array<int, 7> kOffsets{{0, -1, 1, -2, 2, -3, 3}};
  const Cell lava = material_cell(MaterialId::kLava, kLavaMass, kLavaTemperature);
  for (int offset : kOffsets) {
    const int x = static_cast<int>(preferred) + offset;
    if (x < 0 || x >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    if (try_inject(world, x, 0, lava)) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool inject_reaction_pair(World& world, Pcg32& prng) noexcept {
  const std::uint32_t width = static_cast<std::uint32_t>(kWorldWidth - 1U);
  const std::uint32_t height = static_cast<std::uint32_t>(kWorldHeight - 2U);
  const std::uint32_t candidates = width * height;
  const std::uint32_t start = prng.bounded(candidates);

  for (std::uint32_t offset = 0; offset < candidates; ++offset) {
    const std::uint32_t candidate = (start + offset) % candidates;
    const int x = static_cast<int>(candidate % width);
    const int y = 1 + static_cast<int>(candidate / width);
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
      Cell* target = world.try_cell(x + direction[0], y + direction[1]);
      if (target == nullptr || target->material == MaterialId::kCrust ||
          target->material == MaterialId::kMoss) {
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

  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  for (int y = 10; y < static_cast<int>(kWorldHeight); ++y) {
    for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
      static_cast<void>(world.set_cell(x, y, water));
    }
  }

  const Cell lava = material_cell(MaterialId::kLava, kLavaMass, kLavaTemperature);
  for (int y = 0; y <= 3; ++y) {
    for (int x = 6; x <= 9; ++x) {
      static_cast<void>(world.set_cell(x, y, lava));
    }
  }

  const std::uint8_t contact_x = static_cast<std::uint8_t>(7U + prng.bounded(2U));
  static_cast<void>(world.set_cell(contact_x, 8, lava));
  static_cast<void>(world.set_cell(contact_x, 9, lava));
}

LavaWaterSceneStats LavaWaterScene::before_dynamics(LavaWaterTickContext context) const noexcept {
  LavaWaterSceneStats stats{};
  World& world = context.world;
  Pcg32& prng = context.prng;
  const InputFrame& input = context.input;
  WorkBudget& event_budget = context.event_budget;

  if (context.tick != 0U && context.tick % kAutoLavaPeriodTicks == 0U) {
    if (inject_lava_vent(world, prng, stats.last_injection_x)) {
      ++stats.autonomous_lava_injections;
    }
  }

  if (context.tick != 0U && context.tick % kAutoWaterPeriodTicks == 0U) {
    if (refill_water_basin(world, prng, stats.last_injection_x)) {
      ++stats.autonomous_water_injections;
    }
  }

  const bool slider_due = input.slider_active && input.slider_strength >= 0.35F &&
                          context.tick % kTouchLavaPeriodTicks == 0U;
  if (slider_due && event_budget.try_consume()) {
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
