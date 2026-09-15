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
constexpr std::array<int, kWorldWidth> kSodiumWaterSurface{{
    13, 12, 14, 11, 13, 10, 12, 11, 13, 10, 12, 11, 14, 12, 13, 11,
}};
constexpr std::array<int, kWorldWidth> kOilWaterSurface{{
    15, 14, 15, 13, 14, 13, 15, 14, 15, 13, 14, 13, 15, 14, 15, 14,
}};

Cell material_cell(MaterialId material, std::uint8_t mass, std::int16_t temperature = 0,
                   std::uint8_t aux = 0) noexcept {
  Cell cell{};
  cell.material = material;
  cell.mass = mass;
  cell.temperature = temperature;
  cell.aux = aux;
  return cell;
}

Cell layered_water_cell(int x, int y, int surface) noexcept {
  const int depth = y - surface;
  std::uint8_t mass = kWaterMass;
  if (depth == 0) {
    mass = static_cast<std::uint8_t>(122 + (x % 4) * 10);
  } else if (depth == 1) {
    mass = static_cast<std::uint8_t>(170 + (x % 3) * 12);
  }
  return material_cell(MaterialId::kWater, mass, kWaterTemperature);
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
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldWidth));
  for (std::uint32_t offset = 0; offset < kWorldWidth; ++offset) {
    const int x = static_cast<int>((start + offset) % kWorldWidth);
    if (try_inject(world, x, 0, sodium)) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool inject_water_rivulet(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  constexpr std::array<int, 8> kInletXs{{2, 13, 4, 11, 1, 14, 6, 9}};
  const std::uint32_t start = prng.bounded(kInletXs.size());
  for (std::uint32_t offset = 0; offset < kInletXs.size(); ++offset) {
    const int x = kInletXs[(start + offset) % kInletXs.size()];
    const std::uint8_t mass = static_cast<std::uint8_t>(136U + (x % 3) * 14U);
    if (try_inject(world, x, 0, material_cell(MaterialId::kWater, mass, kWaterTemperature))) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
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
    *second = material_cell(MaterialId::kWater, 148, kWaterTemperature);
    return true;
  }
  return false;
}

bool inject_oil_surface(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldWidth));
  for (std::uint32_t offset = 0; offset < kWorldWidth; ++offset) {
    const int x = static_cast<int>((start + offset) % kWorldWidth);
    const int water_surface = kOilWaterSurface[static_cast<std::size_t>(x)];
    for (int rise = 1; rise <= 3; ++rise) {
      const int y = water_surface - rise;
      const std::uint8_t mass = static_cast<std::uint8_t>(154U + ((x + rise) % 3) * 18U);
      if (try_inject(world, x, y, material_cell(MaterialId::kOil, mass, kOilTemperature))) {
        injected_x = static_cast<std::uint8_t>(x);
        return true;
      }
    }
  }
  return false;
}

bool ignite_one_oil(World& world, Pcg32& prng, bool prefer_left) noexcept {
  if (prefer_left) {
    for (std::size_t x = 0; x < kWorldWidth; ++x) {
      for (std::size_t y = 0; y < kWorldHeight; ++y) {
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

  for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
    const int surface = kSodiumWaterSurface[static_cast<std::size_t>(x)];
    for (int y = surface; y < static_cast<int>(kWorldHeight); ++y) {
      static_cast<void>(world.set_cell(x, y, layered_water_cell(x, y, surface)));
    }
  }

  const Cell sodium = material_cell(MaterialId::kSodiumLike, kSodiumMass, kSodiumTemperature);
  constexpr std::array<std::array<int, 2>, 4> kDrops{{
      {{3, 1}}, {{11, 3}}, {{6, 5}}, {{13, 6}},
  }};
  for (const auto& point : kDrops) {
    static_cast<void>(world.set_cell(point[0], point[1], sodium));
  }

  const std::uint8_t contact_x = static_cast<std::uint8_t>(5U + prng.bounded(6U));
  const int contact_y = kSodiumWaterSurface[contact_x] - 1;
  static_cast<void>(world.set_cell(contact_x, contact_y, sodium));
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
    if (inject_water_rivulet(context.world, context.prng, stats.last_injection_x)) {
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

  for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
    const int surface = kOilWaterSurface[static_cast<std::size_t>(x)];
    for (int y = surface; y < static_cast<int>(kWorldHeight); ++y) {
      static_cast<void>(world.set_cell(x, y, layered_water_cell(x, y, surface)));
    }
  }

  for (int x = 0; x < static_cast<int>(kWorldWidth); ++x) {
    const int water_surface = kOilWaterSurface[static_cast<std::size_t>(x)];
    if (x % 4 != 1) {
      const std::uint8_t mass = static_cast<std::uint8_t>(158U + (x % 3) * 18U);
      static_cast<void>(world.set_cell(
          x, water_surface - 1, material_cell(MaterialId::kOil, mass, kOilTemperature)));
    }
    if (x % 3 == 0 || x % 5 == 0) {
      const std::uint8_t mass = static_cast<std::uint8_t>(142U + (x % 2) * 24U);
      static_cast<void>(world.set_cell(
          x, water_surface - 2, material_cell(MaterialId::kOil, mass, kOilTemperature)));
    }
  }

  const Cell fire = material_cell(MaterialId::kFire, 118, 1350, 14);
  for (int x = 0; x <= 2; ++x) {
    const int y = kOilWaterSurface[static_cast<std::size_t>(x)] - 1;
    Cell* cell = world.try_cell(x, y);
    if (cell != nullptr && cell->material == MaterialId::kOil) {
      *cell = fire;
    }
  }
  if (prng.bounded(2U) != 0U) {
    const int y = kOilWaterSurface[3] - 2;
    Cell* cell = world.try_cell(3, y);
    if (cell != nullptr && cell->material == MaterialId::kOil) {
      *cell = fire;
    }
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
