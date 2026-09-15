#include <espsand/sim/energetic_scenes.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace espsand::sim {
namespace {

using TickContext = EnergeticSceneTickContext;

constexpr std::uint64_t kSodiumAutoPeriodTicks = 180;
constexpr std::uint64_t kSodiumWaterRefillPeriodTicks = 300;
constexpr std::uint64_t kTouchInjectionPeriodTicks = 12;
constexpr std::uint64_t kOilCycleTicks = 480;
constexpr std::uint64_t kOilRefillStartTick = 240;
constexpr std::uint64_t kOilIgnitionTick = 300;
constexpr std::uint8_t kSodiumMass = 184;
constexpr std::uint8_t kWaterMass = 220;
constexpr std::uint8_t kOilMass = 208;
constexpr std::int16_t kSodiumTemperature = 100;
constexpr std::int16_t kWaterTemperature = 20;
constexpr std::int16_t kOilTemperature = 28;

Cell material_cell(MaterialId material, std::uint8_t mass, std::int16_t temperature = 0,
                   std::uint8_t aux = 0) noexcept {
  Cell cell{};
  cell.material = material;
  cell.mass = mass;
  cell.temperature = temperature;
  cell.aux = aux;
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

bool inject_selected_top(World& world, float position, const Cell& material,
                         std::uint8_t& injected_x) noexcept {
  const std::uint8_t preferred = slider_to_x(position);
  constexpr std::array<int, 7> kOffsets{{0, -1, 1, -2, 2, -3, 3}};
  for (int offset : kOffsets) {
    const int x = static_cast<int>(preferred) + offset;
    if (x < 0 || x >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    if (try_inject(world, x, 0, material)) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool inject_sodium_drop(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  const Cell sodium = material_cell(MaterialId::kSodiumLike, kSodiumMass, kSodiumTemperature);
  const std::uint32_t width = static_cast<std::uint32_t>(kWorldWidth - 1U);
  const std::uint32_t start = prng.bounded(width);
  for (std::uint32_t offset = 0; offset < width; ++offset) {
    const int x = static_cast<int>((start + offset) % width);
    Cell* first = world.try_cell(x, 0);
    Cell* second = world.try_cell(x + 1, 0);
    if (first == nullptr || second == nullptr || !can_inject_into(*first) ||
        !can_inject_into(*second)) {
      continue;
    }
    *first = sodium;
    *second = sodium;
    injected_x = static_cast<std::uint8_t>(x);
    return true;
  }
  return false;
}

bool refill_water(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldWidth));
  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  for (int y = static_cast<int>(kWorldHeight) - 1; y >= 9; --y) {
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

bool inject_sodium_water_pair(World& world, Pcg32& prng) noexcept {
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

    *first = material_cell(MaterialId::kSodiumLike, kSodiumMass, kSodiumTemperature);
    *second = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
    return true;
  }
  return false;
}

bool inject_oil_surface(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  const Cell oil = material_cell(MaterialId::kOil, kOilMass, kOilTemperature);
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldWidth));
  for (std::uint32_t offset = 0; offset < kWorldWidth; ++offset) {
    const int x = static_cast<int>((start + offset) % kWorldWidth);
    if (try_inject(world, x, 8, oil)) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool ignite_one_oil(World& world, Pcg32& prng, bool prefer_left) noexcept {
  if (prefer_left) {
    for (std::size_t x = 0; x < kWorldWidth; ++x) {
      for (std::size_t y = 8; y < kWorldHeight; ++y) {
        Cell* cell = world.try_cell(static_cast<int>(x), static_cast<int>(y));
        if (cell == nullptr || cell->material != MaterialId::kOil || cell->mass == 0U) {
          continue;
        }
        cell->material = MaterialId::kFire;
        cell->temperature = std::max<std::int16_t>(cell->temperature, 1200);
        cell->aux = std::max<std::uint8_t>(cell->aux, 14U);
        return true;
      }
    }
    return false;
  }

  const std::uint32_t capacity = static_cast<std::uint32_t>(kWorldCapacity);
  const std::uint32_t start = prng.bounded(capacity);
  for (std::uint32_t offset = 0; offset < capacity; ++offset) {
    Cell* cell = world.try_cell_index((start + offset) % capacity);
    if (cell == nullptr || cell->material != MaterialId::kOil || cell->mass == 0U) {
      continue;
    }
    cell->material = MaterialId::kFire;
    cell->temperature = std::max<std::int16_t>(cell->temperature, 1200);
    cell->aux = std::max<std::uint8_t>(cell->aux, 14U);
    return true;
  }
  return false;
}

bool oil_refill_phase(std::uint64_t tick) noexcept {
  const std::uint64_t phase = tick % kOilCycleTicks;
  if (phase < kOilRefillStartTick || phase >= kOilIgnitionTick) {
    return false;
  }
  return (phase - kOilRefillStartTick) % 12U == 0U;
}

} // namespace

void SodiumWaterScene::initialize(World& world, Pcg32& prng) const noexcept {
  world.clear();

  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  for (int y = 9; y < static_cast<int>(kWorldHeight); ++y) {
    for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
      static_cast<void>(world.set_cell(x, y, water));
    }
  }

  const Cell sodium = material_cell(MaterialId::kSodiumLike, kSodiumMass, kSodiumTemperature);
  static_cast<void>(world.set_cell(3, 1, sodium));
  static_cast<void>(world.set_cell(4, 1, sodium));
  static_cast<void>(world.set_cell(10, 3, sodium));
  static_cast<void>(world.set_cell(11, 3, sodium));

  const std::uint8_t contact_x = static_cast<std::uint8_t>(6U + prng.bounded(4U));
  static_cast<void>(world.set_cell(contact_x, 8, sodium));
  static_cast<void>(world.set_cell(contact_x + 1U, 8, sodium));
}

SodiumWaterSceneStats SodiumWaterScene::before_dynamics(TickContext context) const noexcept {
  SodiumWaterSceneStats stats{};
  const Cell sodium = material_cell(MaterialId::kSodiumLike, kSodiumMass, kSodiumTemperature);

  if (context.tick != 0U && context.tick % kSodiumAutoPeriodTicks == 0U) {
    if (inject_sodium_drop(context.world, context.prng, stats.last_injection_x)) {
      ++stats.autonomous_sodium_injections;
    }
  }

  if (context.tick != 0U && context.tick % kSodiumWaterRefillPeriodTicks == 0U) {
    if (refill_water(context.world, context.prng, stats.last_injection_x)) {
      ++stats.autonomous_water_injections;
    }
  }

  const bool slider_injection = context.input.slider_active &&
                                context.input.slider_strength >= 0.35F &&
                                context.tick % kTouchInjectionPeriodTicks == 0U;
  if (slider_injection && context.event_budget.try_consume()) {
    if (inject_selected_top(context.world, context.input.slider_position, sodium,
                            stats.last_injection_x)) {
      ++stats.touch_sodium_injections;
    }
  }

  if (context.input.cap_combo_event && context.event_budget.try_consume()) {
    if (inject_sodium_water_pair(context.world, context.prng)) {
      ++stats.burst_pairs;
    }
  }

  return stats;
}

void OilFireScene::initialize(World& world, Pcg32& prng) const noexcept {
  world.clear();

  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  for (int y = 13; y < static_cast<int>(kWorldHeight); ++y) {
    for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
      static_cast<void>(world.set_cell(x, y, water));
    }
  }

  const Cell oil = material_cell(MaterialId::kOil, kOilMass, kOilTemperature);
  for (int y = 9; y <= 12; ++y) {
    for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
      static_cast<void>(world.set_cell(x, y, oil));
    }
  }

  const Cell fire = material_cell(MaterialId::kFire, 128, 1350, 14);
  static_cast<void>(world.set_cell(0, 10, fire));
  static_cast<void>(world.set_cell(1, 10, fire));
  if (prng.bounded(2U) != 0U) {
    static_cast<void>(world.set_cell(0, 9, fire));
  }
}

OilFireSceneStats OilFireScene::before_dynamics(TickContext context) const noexcept {
  OilFireSceneStats stats{};
  const Cell oil = material_cell(MaterialId::kOil, kOilMass, kOilTemperature);

  if (oil_refill_phase(context.tick)) {
    if (inject_oil_surface(context.world, context.prng, stats.last_injection_x)) {
      ++stats.autonomous_oil_injections;
    }
  }

  if (context.tick != 0U && context.tick % kOilCycleTicks == kOilIgnitionTick) {
    if (ignite_one_oil(context.world, context.prng, true)) {
      ++stats.autonomous_ignitions;
    }
  }

  const bool slider_injection = context.input.slider_active &&
                                context.input.slider_strength >= 0.35F &&
                                context.tick % kTouchInjectionPeriodTicks == 0U;
  if (slider_injection && context.event_budget.try_consume()) {
    if (inject_selected_top(context.world, context.input.slider_position, oil,
                            stats.last_injection_x)) {
      ++stats.touch_oil_injections;
    }
  }

  if (context.input.cap_combo_event && context.event_budget.try_consume()) {
    if (ignite_one_oil(context.world, context.prng, false)) {
      ++stats.combo_ignitions;
    }
  }

  return stats;
}

} // namespace espsand::sim
