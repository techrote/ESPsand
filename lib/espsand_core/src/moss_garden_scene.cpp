#include <espsand/sim/moss_garden_scene.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <espsand/sim/scene_limits.hpp>

namespace espsand::sim {
namespace {

constexpr std::uint16_t kMaxGrowthEnergy = 512;
constexpr std::uint64_t kGrowthChargePeriodTicks = 6;
constexpr std::uint64_t kGrowthPeriodTicks = 10;
constexpr std::uint64_t kAutonomousRainPeriodTicks = 210;
constexpr std::uint64_t kTouchMitePeriodTicks = 12;
constexpr std::uint8_t kMossInitialMass = 150;
constexpr std::uint8_t kMossInitialAux = 132;
constexpr std::uint8_t kMiteInitialEnergy = 96;
constexpr std::uint8_t kMiteMaximumEnergy = 200;
constexpr std::uint8_t kFeedAmount = 22;
constexpr std::array<std::array<int, 2>, 4> kDirections{{
    {{1, 0}},
    {{-1, 0}},
    {{0, 1}},
    {{0, -1}},
}};

Cell material_cell(MaterialId material, std::uint8_t mass, std::uint8_t aux = 0,
                   std::uint8_t flags = 0) noexcept {
  Cell cell{};
  cell.material = material;
  cell.mass = mass;
  cell.aux = aux;
  cell.flags = flags;
  return cell;
}

bool is_water(const Cell* cell) noexcept {
  return cell != nullptr && cell->material == MaterialId::kWater && cell->mass != 0U;
}

bool is_moss(const Cell* cell) noexcept {
  return cell != nullptr && cell->material == MaterialId::kMoss && cell->mass != 0U;
}

std::uint8_t moisture_score(const World& world, int x, int y) noexcept {
  std::uint8_t score = 0;
  for (int dy = -2; dy <= 2; ++dy) {
    for (int dx = -2; dx <= 2; ++dx) {
      if (std::abs(dx) + std::abs(dy) > 2) {
        continue;
      }
      if (is_water(world.try_cell(x + dx, y + dy))) {
        score = static_cast<std::uint8_t>(std::min<unsigned>(255U, score + 24U));
      }
    }
  }
  return score;
}

std::array<int, 2> anti_gravity_direction(const InputFrame& input) noexcept {
  if (input.gravity_confidence < 0.20F || input.gravity_magnitude < 0.05F) {
    return {{0, -1}};
  }
  if (std::fabs(input.gravity.x) > std::fabs(input.gravity.y)) {
    return {{input.gravity.x > 0.0F ? -1 : 1, 0}};
  }
  return {{0, input.gravity.y > 0.0F ? -1 : 1}};
}

bool can_grow_into(const Cell* cell) noexcept {
  return cell != nullptr && (cell->material == MaterialId::kEmpty || cell->mass == 0U);
}

void add_growth_energy(std::uint16_t& energy, std::uint16_t amount) noexcept {
  energy = static_cast<std::uint16_t>(
      std::min<std::uint32_t>(kMaxGrowthEnergy, static_cast<std::uint32_t>(energy) + amount));
}

bool grow_from(World& world, const InputFrame& input, std::size_t source_index,
               std::uint16_t& growth_energy, MossGardenSceneStats& stats) noexcept {
  Cell* source = world.try_cell_index(source_index);
  if (!is_moss(source)) {
    return false;
  }

  const int x = static_cast<int>(source_index % kWorldWidth);
  const int y = static_cast<int>(source_index / kWorldWidth);
  const std::uint8_t source_moisture = moisture_score(world, x, y);
  if (source_moisture == 0U) {
    return false;
  }

  const bool may_add_moss = can_add_product_material(world, MaterialId::kMoss);
  const auto shoot = anti_gravity_direction(input);
  if (may_add_moss && source->aux >= 140U && growth_energy >= 32U) {
    Cell* target = world.try_cell(x + shoot[0], y + shoot[1]);
    if (can_grow_into(target)) {
      *target = material_cell(MaterialId::kMoss, 92, 168, kMossShootFlag);
      growth_energy = static_cast<std::uint16_t>(growth_energy - 32U);
      ++stats.growth_cells;
      return true;
    }
  }

  int best_x = x;
  int best_y = y;
  std::uint8_t best_moisture = 0;
  if (may_add_moss) {
    for (const auto& direction : kDirections) {
      const int target_x = x + direction[0];
      const int target_y = y + direction[1];
      const Cell* target = world.try_cell(target_x, target_y);
      if (!can_grow_into(target)) {
        continue;
      }
      const std::uint8_t moisture = moisture_score(world, target_x, target_y);
      if (moisture > best_moisture) {
        best_moisture = moisture;
        best_x = target_x;
        best_y = target_y;
      }
    }
  }

  if (may_add_moss && growth_energy >= 24U &&
      (best_moisture != 0U || source_moisture >= 48U) && (best_x != x || best_y != y)) {
    Cell* target = world.try_cell(best_x, best_y);
    if (target != nullptr) {
      *target = material_cell(MaterialId::kMoss, 78, 104);
      growth_energy = static_cast<std::uint16_t>(growth_energy - 24U);
      ++stats.growth_cells;
      return true;
    }
  }

  if (growth_energy >= 12U && source->mass < 236U) {
    source->mass = static_cast<std::uint8_t>(std::min<unsigned>(255U, source->mass + 18U));
    source->aux = static_cast<std::uint8_t>(std::min<unsigned>(255U, source->aux + 10U));
    growth_energy = static_cast<std::uint16_t>(growth_energy - 12U);
    ++stats.reinforced_cells;
    return true;
  }
  return false;
}

std::uint8_t active_mite_count(const std::array<MiteState, kMaxMites>& mites) noexcept {
  std::uint8_t count = 0;
  for (const MiteState& mite : mites) {
    if (mite.active) {
      ++count;
    }
  }
  return count;
}

bool mite_step_allowed(const World& world, int x, int y) noexcept {
  const Cell* cell = world.try_cell(x, y);
  if (cell == nullptr) {
    return false;
  }
  return cell->material != MaterialId::kWater && cell->material != MaterialId::kLava &&
         cell->material != MaterialId::kOil && cell->material != MaterialId::kFire;
}

bool move_mite_to(MiteState& mite, const World& world, int x, int y) noexcept {
  if (!mite_step_allowed(world, x, y)) {
    return false;
  }
  mite.x = static_cast<std::uint8_t>(x);
  mite.y = static_cast<std::uint8_t>(y);
  return true;
}

bool find_nearest_moss(const World& world, const MiteState& mite, int& target_x,
                       int& target_y) noexcept {
  int best_distance = 1000;
  bool found = false;
  for (std::size_t index = 0; index < kWorldCapacity; ++index) {
    const Cell* cell = world.try_cell_index(index);
    if (!is_moss(cell)) {
      continue;
    }
    const int x = static_cast<int>(index % kWorldWidth);
    const int y = static_cast<int>(index / kWorldWidth);
    const int delta_x = x - static_cast<int>(mite.x);
    const int delta_y = y - static_cast<int>(mite.y);
    const int distance = std::abs(delta_x) + std::abs(delta_y);
    if (distance < best_distance) {
      best_distance = distance;
      target_x = x;
      target_y = y;
      found = true;
    }
  }
  return found;
}

bool move_toward_moss(MiteState& mite, const World& world, Pcg32& prng) noexcept {
  int target_x = 0;
  int target_y = 0;
  if (find_nearest_moss(world, mite, target_x, target_y)) {
    const int dx = target_x - static_cast<int>(mite.x);
    const int dy = target_y - static_cast<int>(mite.y);
    const bool x_first = std::abs(dx) >= std::abs(dy);
    const int step_x = dx == 0 ? 0 : (dx > 0 ? 1 : -1);
    const int step_y = dy == 0 ? 0 : (dy > 0 ? 1 : -1);
    if (x_first && step_x != 0 &&
        move_mite_to(mite, world, static_cast<int>(mite.x) + step_x, mite.y)) {
      return true;
    }
    const int next_y = static_cast<int>(mite.y) + step_y;
    if (step_y != 0 && move_mite_to(mite, world, mite.x, next_y)) {
      return true;
    }
    if (!x_first && step_x != 0 &&
        move_mite_to(mite, world, static_cast<int>(mite.x) + step_x, mite.y)) {
      return true;
    }
  }

  const std::uint32_t start = prng.bounded(kDirections.size());
  for (std::uint32_t offset = 0; offset < kDirections.size(); ++offset) {
    const auto& direction = kDirections[(start + offset) % kDirections.size()];
    if (move_mite_to(mite, world, static_cast<int>(mite.x) + direction[0],
                     static_cast<int>(mite.y) + direction[1])) {
      return true;
    }
  }
  return false;
}

void feed_mite(MiteState& mite, World& world, MossGardenSceneStats& stats) noexcept {
  Cell* cell = world.try_cell(mite.x, mite.y);
  if (!is_moss(cell)) {
    return;
  }

  if (cell->mass <= kFeedAmount) {
    *cell = Cell{};
  } else {
    cell->mass = static_cast<std::uint8_t>(cell->mass - kFeedAmount);
    cell->aux = cell->aux > 6U ? static_cast<std::uint8_t>(cell->aux - 6U) : 0U;
  }
  mite.energy = static_cast<std::uint8_t>(
      std::min<unsigned>(kMiteMaximumEnergy, static_cast<unsigned>(mite.energy) + 18U));
  ++stats.feeds;
}

std::uint8_t slider_x(float position) noexcept {
  const float scaled = position * static_cast<float>(kWorldWidth - 1U);
  return static_cast<std::uint8_t>(
      std::min<std::uint32_t>(kWorldWidth - 1U, static_cast<std::uint32_t>(scaled + 0.5F)));
}

std::uint16_t inject_rain(World& world, std::uint8_t preferred_x) noexcept {
  constexpr std::array<int, 7> kOffsets{{0, -1, 1, -2, 2, -3, 3}};
  std::uint16_t injected = 0;
  for (int offset : kOffsets) {
    const int x = static_cast<int>(preferred_x) + offset;
    if (x < 0 || x >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    for (int y = 0; y <= 1; ++y) {
      if (!can_add_product_material(world, MaterialId::kWater)) {
        return injected;
      }
      Cell* cell = world.try_cell(x, y);
      if (cell == nullptr || (cell->material != MaterialId::kEmpty && cell->mass != 0U)) {
        continue;
      }
      *cell = material_cell(MaterialId::kWater, 186);
      ++injected;
      if (injected == 2U) {
        return injected;
      }
    }
  }
  return injected;
}

bool place_mite_near_x(MiteState& mite, const World& world, std::uint8_t preferred_x) noexcept {
  constexpr std::array<int, 11> kOffsets{{0, -1, 1, -2, 2, -3, 3, -4, 4, -5, 5}};
  for (int offset : kOffsets) {
    const int x = static_cast<int>(preferred_x) + offset;
    if (x < 0 || x >= static_cast<int>(kWorldWidth)) {
      continue;
    }
    for (int y = 0; y < static_cast<int>(kWorldHeight); ++y) {
      if (!is_moss(world.try_cell(x, y))) {
        continue;
      }
      mite.x = static_cast<std::uint8_t>(x);
      mite.y = static_cast<std::uint8_t>(y);
      mite.energy = 84U;
      mite.active = true;
      return true;
    }
  }
  return false;
}

bool seed_moss_near_water(World& world, Pcg32& prng) noexcept {
  if (!can_add_product_material(world, MaterialId::kMoss)) {
    return false;
  }
  const std::uint32_t start = prng.bounded(static_cast<std::uint32_t>(kWorldCapacity));
  for (std::uint32_t offset = 0; offset < kWorldCapacity; ++offset) {
    const std::size_t index = (start + offset) % kWorldCapacity;
    Cell* cell = world.try_cell_index(index);
    if (!can_grow_into(cell)) {
      continue;
    }
    const int x = static_cast<int>(index % kWorldWidth);
    const int y = static_cast<int>(index / kWorldWidth);
    if (moisture_score(world, x, y) == 0U) {
      continue;
    }
    *cell = material_cell(MaterialId::kMoss, 86, 116);
    return true;
  }
  return false;
}

float disturbance_strength(const InputFrame& input) noexcept {
  return std::max({input.shake_energy, input.motion_energy, input.tap_impulse,
                   input.noise_event ? input.noise_impulse : 0.0F});
}

} // namespace

void MossGardenScene::initialize(World& world, Pcg32& prng) noexcept {
  world.clear();
  mites_ = {};
  mite_count_ = 0;
  growth_energy_ = 36;

  const Cell water = material_cell(MaterialId::kWater, 188);
  constexpr std::array<std::array<int, 2>, 8> kWaterPockets{{
      {{1, 15}}, {{3, 13}}, {{5, 15}}, {{7, 14}},
      {{9, 15}}, {{11, 13}}, {{13, 15}}, {{15, 14}},
  }};
  for (const auto& point : kWaterPockets) {
    static_cast<void>(world.set_cell(point[0], point[1], water));
  }

  const Cell moss = material_cell(MaterialId::kMoss, kMossInitialMass, kMossInitialAux);
  constexpr std::array<std::array<int, 2>, 10> kMossPatches{{
      {{1, 12}}, {{2, 11}}, {{3, 12}}, {{5, 12}}, {{6, 11}},
      {{9, 12}}, {{10, 11}}, {{12, 12}}, {{13, 11}}, {{14, 12}},
  }};
  for (const auto& point : kMossPatches) {
    static_cast<void>(world.set_cell(point[0], point[1], moss));
  }
  static_cast<void>(world.set_cell(2, 10,
                                   material_cell(MaterialId::kMoss, 132, 172, kMossShootFlag)));
  static_cast<void>(world.set_cell(13, 10,
                                   material_cell(MaterialId::kMoss, 124, 184, kMossShootFlag)));

  mites_[0] = {2, 11, kMiteInitialEnergy, true};
  if (prng.bounded(2U) != 0U) {
    mites_[0].x = 3;
  }
  mite_count_ = active_mite_count(mites_);
}

MossGardenSceneStats MossGardenScene::before_dynamics(MossGardenTickContext context) noexcept {
  MossGardenSceneStats stats{};

  if (context.tick % kGrowthChargePeriodTicks == 0U) {
    std::uint16_t wet_moss = 0;
    for (std::size_t index = 0; index < kWorldCapacity; ++index) {
      const Cell* cell = context.world.try_cell_index(index);
      if (!is_moss(cell)) {
        continue;
      }
      const int x = static_cast<int>(index % kWorldWidth);
      const int y = static_cast<int>(index / kWorldWidth);
      if (moisture_score(context.world, x, y) != 0U) {
        ++wet_moss;
      }
    }
    const unsigned gain = std::min<unsigned>(30U, wet_moss * 3U);
    add_growth_energy(growth_energy_, static_cast<std::uint16_t>(gain));
  }

  if (context.tick % kGrowthPeriodTicks == 0U && growth_energy_ >= 12U) {
    const std::uint32_t start = context.prng.bounded(static_cast<std::uint32_t>(kWorldCapacity));
    std::uint16_t completed = 0;
    for (std::uint32_t offset = 0; offset < kWorldCapacity && completed < 2U; ++offset) {
      const std::size_t index = (start + offset) % kWorldCapacity;
      if (grow_from(context.world, context.input, index, growth_energy_, stats)) {
        ++completed;
      }
    }
  }

  for (MiteState& mite : mites_) {
    if (!mite.active) {
      continue;
    }
    if (context.tick % 3U == 0U && mite.energy != 0U) {
      --mite.energy;
    }
    if (context.tick % 4U == 0U) {
      feed_mite(mite, context.world, stats);
    }
    if (context.tick % 2U == 0U && move_toward_moss(mite, context.world, context.prng)) {
      ++stats.mite_moves;
    }
    if (mite.energy == 0U) {
      mite.active = false;
      ++stats.starved;
    }
  }

  if (context.tick != 0U && context.tick % kAutonomousRainPeriodTicks == 0U) {
    const auto width = static_cast<std::uint32_t>(kWorldWidth);
    const auto x = static_cast<std::uint8_t>(context.prng.bounded(width));
    if (inject_rain(context.world, x) != 0U) {
      ++stats.rain_pulses;
    }
  }

  const bool slider_due = context.input.slider_active && context.input.slider_strength >= 0.35F &&
                          context.tick % kTouchMitePeriodTicks == 0U;
  if (slider_due && context.event_budget.try_consume()) {
    for (MiteState& mite : mites_) {
      if (!mite.active && place_mite_near_x(mite, context.world,
                                            slider_x(context.input.slider_position))) {
        ++stats.touch_mite_spawns;
        break;
      }
    }
  }

  if (context.input.cap_combo_event && context.event_budget.try_consume()) {
    if (seed_moss_near_water(context.world, context.prng)) {
      ++stats.seed_pulses;
    }
  }

  if (disturbance_strength(context.input) >= 0.70F && context.event_budget.try_consume()) {
    for (MiteState& mite : mites_) {
      if (!mite.active) {
        continue;
      }
      const std::uint32_t start = context.prng.bounded(kDirections.size());
      for (std::uint32_t offset = 0; offset < kDirections.size(); ++offset) {
        const auto& direction = kDirections[(start + offset) % kDirections.size()];
        if (move_mite_to(mite, context.world, static_cast<int>(mite.x) + direction[0],
                         static_cast<int>(mite.y) + direction[1])) {
          break;
        }
      }
    }
    ++stats.scatter_events;
  }

  mite_count_ = active_mite_count(mites_);
  return stats;
}

MossGardenStateSnapshot MossGardenScene::snapshot() const noexcept {
  return {mites_, mite_count_, growth_energy_};
}

bool MossGardenScene::invariants_hold(const World& world) const noexcept {
  if (growth_energy_ > kMaxGrowthEnergy || mite_count_ > kMaxMites) {
    return false;
  }
  if (material_cell_count(world, MaterialId::kMoss) > kProductMaterialCellLimit ||
      material_cell_count(world, MaterialId::kWater) > kProductMaterialCellLimit) {
    return false;
  }
  std::uint8_t active = 0;
  for (const MiteState& mite : mites_) {
    if (!mite.active) {
      continue;
    }
    ++active;
    if (mite.energy > kMiteMaximumEnergy || !world.in_bounds(mite.x, mite.y)) {
      return false;
    }
  }
  return active == mite_count_;
}

} // namespace espsand::sim
