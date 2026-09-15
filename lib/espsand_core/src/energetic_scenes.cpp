#include <espsand/sim/energetic_scenes.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/sim/scene_limits.hpp>

namespace espsand::sim {
namespace {

using TickContext = EnergeticSceneTickContext;

constexpr std::uint64_t kSodiumAutoPeriodTicks = 220;
constexpr std::uint64_t kSodiumWaterRefillPeriodTicks = 360;
constexpr std::uint64_t kTouchInjectionPeriodTicks = 12;
constexpr std::uint64_t kOilCycleTicks = 520;
constexpr std::uint64_t kOilRefillStartTick = 280;
constexpr std::uint64_t kOilIgnitionTick = 340;
constexpr std::uint8_t kSodiumMass = 184;
constexpr std::uint8_t kWaterMass = 188;
constexpr std::uint8_t kOilMass = 190;
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
  if (!can_add_product_material(world, material.material)) {
    return false;
  }
  const std::uint8_t preferred = slider_to_x(position);
  constexpr std::array<int, 7> kOffsets{{0, -1, 1, -2, 2, -3, 3}};
  for (int offset : kOffsets) {
    const int x = static_cast<int>(preferred) + offset;
    if (x < 0 || x >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    for (int y = 0; y <= 2; ++y) {
      if (try_inject(world, x, y, material)) {
        injected_x = static_cast<std::uint8_t>(x);
        return true;
      }
    }
  }
  return false;
}

bool inject_sodium_drop(World& world, Pcg32& prng, std::uint8_t& injected_x) noexcept {
  if (!can_add_product_material(world, MaterialId::kSodiumLike)) {
    return false;
  }
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
  if (!can_add_product_material(world, MaterialId::kWater)) {
    return false;
  }
  constexpr std::array<int, 8> kInletXs{{2, 13, 4, 11, 1, 14, 6, 9}};
  const std::uint32_t start = prng.bounded(kInletXs.size());
  for (std::uint32_t offset = 0; offset < kInletXs.size(); ++offset) {
    const int x = kInletXs[(start + offset) % kInletXs.size()];
    const std::uint8_t mass = static_cast<std::uint8_t>(140U + (x % 3) * 14U);
    if (try_inject(world, x, 0, material_cell(MaterialId::kWater, mass, kWaterTemperature))) {
      injected_x = static_cast<std::uint8_t>(x);
      return true;
    }
  }
  return false;
}

bool inject_sodium_water_pair(World& world, Pcg32& prng) noexcept {
  if (!can_add_product_material(world, MaterialId::kSodiumLike) ||
      !can_add_product_material(world, MaterialId::kWater)) {
    return false;
  }
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
  if (!can_add_product_material(world, MaterialId::kOil)) {
    return false;
  }
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldWidth));
  for (std::uint32_t offset = 0; offset < kWorldWidth; ++offset) {
    const int x = static_cast<int>((start + offset) % kWorldWidth);
    constexpr std::array<int, 3> kRows{{10, 8, 12}};
    for (int y : kRows) {
      const std::uint8_t mass = static_cast<std::uint8_t>(154U + ((x + y) % 3) * 18U);
      if (try_inject(world, x, y, material_cell(MaterialId::kOil, mass, kOilTemperature))) {
        injected_x = static_cast<std::uint8_t>(x);
        return true;
      }
    }
  }
  return false;
}

bool ignite_cell(Cell& cell) noexcept {
  if (cell.material != MaterialId::kOil || cell.mass == 0U) {
    return false;
  }
  cell.material = MaterialId::kFire;
  cell.temperature = std::max<std::int16_t>(cell.temperature, 1200);
  cell.aux = std::max<std::uint8_t>(cell.aux, 14U);
  return true;
}

bool ignite_oil_near_x(World& world, float position, std::uint8_t& ignited_x) noexcept {
  const int preferred = slider_to_x(position);
  constexpr std::array<int, 9> kOffsets{{0, -1, 1, -2, 2, -3, 3, -4, 4}};
  for (int offset : kOffsets) {
    const int x = preferred + offset;
    if (x < 0 || x >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    for (int y = 0; y < static_cast<int>(kWorldHeight); ++y) {
      Cell* cell = world.try_cell(x, y);
      if (cell != nullptr && ignite_cell(*cell)) {
        ignited_x = static_cast<std::uint8_t>(x);
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
        if (cell != nullptr && ignite_cell(*cell)) {
          return true;
        }
      }
    }
    return false;
  }

  const std::uint32_t capacity = static_cast<std::uint32_t>(kWorldCapacity);
  const std::uint32_t start = prng.bounded(capacity);
  for (std::uint32_t offset = 0; offset < capacity; ++offset) {
    Cell* cell = world.try_cell_index((start + offset) % capacity);
    if (cell != nullptr && ignite_cell(*cell)) {
      return true;
    }
  }
  return false;
}

bool oil_refill_phase(std::uint64_t tick) noexcept {
  const std::uint64_t phase = tick % kOilCycleTicks;
  if (phase < kOilRefillStartTick || phase >= kOilIgnitionTick) {
    return false;
  }
  return (phase - kOilRefillStartTick) % 15U == 0U;
}

} // namespace

void SodiumWaterScene::initialize(World& world, Pcg32& prng) const noexcept {
  world.clear();

  const Cell water = material_cell(MaterialId::kWater, kWaterMass, kWaterTemperature);
  // clang-format off
  constexpr std::array<std::array<int, 2>, 12> kWaterPockets{{
      {{0, 15}}, {{2, 13}}, {{3, 15}}, {{5, 12}}, {{6, 14}}, {{7, 15}},
      {{8, 12}}, {{10, 14}}, {{11, 15}}, {{13, 13}}, {{14, 15}}, {{15, 12}},
  }};
  // clang-format on
  for (std::size_t index = 0; index < kWaterPockets.size(); ++index) {
    Cell pocket = water;
    pocket.mass = static_cast<std::uint8_t>(146U + (index % 3U) * 18U);
    const auto& point = kWaterPockets[index];
    static_cast<void>(world.set_cell(point[0], point[1], pocket));
  }

  const Cell sodium = material_cell(MaterialId::kSodiumLike, kSodiumMass, kSodiumTemperature);
  // clang-format off
  constexpr std::array<std::array<int, 2>, 4> kDrops{{
      {{3, 1}}, {{11, 4}}, {{6, 7}}, {{8, 11}},
  }};
  // clang-format on
  for (const auto& point : kDrops) {
    static_cast<void>(world.set_cell(point[0], point[1], sodium));
  }

  if (prng.bounded(2U) != 0U) {
    Cell* cell = world.try_cell(8, 11);
    if (cell != nullptr) {
      cell->mass = 200U;
    }
  }
}

SodiumWaterSceneStats SodiumWaterScene::before_dynamics(TickContext context) const noexcept {
  SodiumWaterSceneStats stats{};

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
    const Cell water = material_cell(MaterialId::kWater, 176, kWaterTemperature);
    if (inject_selected_top(context.world, context.input.slider_position, water,
                            stats.last_injection_x)) {
      ++stats.touch_water_injections;
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
  // clang-format off
  constexpr std::array<std::array<int, 2>, 8> kWaterPockets{{
      {{1, 15}}, {{3, 13}}, {{5, 15}}, {{7, 14}},
      {{9, 15}}, {{11, 13}}, {{13, 15}}, {{15, 14}},
  }};
  // clang-format on
  for (const auto& point : kWaterPockets) {
    static_cast<void>(world.set_cell(point[0], point[1], water));
  }

  // clang-format off
  constexpr std::array<std::array<int, 2>, 10> kOilPockets{{
      {{0, 11}}, {{2, 10}}, {{4, 12}}, {{5, 9}}, {{7, 11}},
      {{8, 8}}, {{10, 10}}, {{12, 12}}, {{13, 9}}, {{15, 11}},
  }};
  // clang-format on
  for (std::size_t index = 0; index < kOilPockets.size(); ++index) {
    const std::uint8_t mass = static_cast<std::uint8_t>(154U + (index % 3U) * 18U);
    const Cell oil = material_cell(MaterialId::kOil, mass, kOilTemperature);
    const auto& point = kOilPockets[index];
    static_cast<void>(world.set_cell(point[0], point[1], oil));
  }

  Cell* ignition = world.try_cell(0, 11);
  if (ignition != nullptr) {
    static_cast<void>(ignite_cell(*ignition));
  }
  if (prng.bounded(3U) == 0U) {
    Cell* second = world.try_cell(2, 10);
    if (second != nullptr) {
      static_cast<void>(ignite_cell(*second));
    }
  }
}

OilFireSceneStats OilFireScene::before_dynamics(TickContext context) const noexcept {
  OilFireSceneStats stats{};

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
    if (ignite_oil_near_x(context.world, context.input.slider_position, stats.last_injection_x)) {
      ++stats.touch_ignitions;
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
