#include <espsand/sim/dynamics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace espsand::sim {
namespace {

struct Direction {
  int dx = 0;
  int dy = 0;
  bool active = false;
};

std::uint8_t unit_to_q8(float value) noexcept {
  return static_cast<std::uint8_t>(value * 255.0F + 0.5F);
}

std::int8_t decay_motion(std::int8_t value) noexcept {
  if (value > 0) {
    return static_cast<std::int8_t>(std::max(0, static_cast<int>(value) - 8));
  }
  if (value < 0) {
    return static_cast<std::int8_t>(std::min(0, static_cast<int>(value) + 8));
  }
  return 0;
}

std::int16_t clamp_temperature(std::int32_t value) noexcept {
  const std::int32_t minimum = std::numeric_limits<std::int16_t>::min();
  const std::int32_t maximum = std::numeric_limits<std::int16_t>::max();
  return static_cast<std::int16_t>(std::clamp(value, minimum, maximum));
}

Direction choose_gravity_direction(const InputFrame& frame, std::uint64_t tick) noexcept {
  if (frame.gravity_confidence < 0.20F || frame.gravity_magnitude < 0.05F) {
    return {};
  }

  const std::uint32_t x_weight = unit_to_q8(std::fabs(frame.gravity.x));
  const std::uint32_t y_weight = unit_to_q8(std::fabs(frame.gravity.y));
  const std::uint32_t total = x_weight + y_weight;
  if (total == 0U) {
    return {};
  }

  const std::uint32_t phase = static_cast<std::uint32_t>(tick % total);
  if (x_weight != 0U && phase < x_weight) {
    return {frame.gravity.x < 0.0F ? -1 : 1, 0, true};
  }
  if (y_weight != 0U) {
    return {0, frame.gravity.y < 0.0F ? -1 : 1, true};
  }
  return {frame.gravity.x < 0.0F ? -1 : 1, 0, true};
}

std::uint8_t disturbance_strength(const InputFrame& frame) noexcept {
  const float maximum = std::max(
      {frame.shake_energy, frame.motion_energy, frame.tap_impulse, std::fabs(frame.spin_rate)});
  return unit_to_q8(maximum);
}

bool mobility_allows(std::uint8_t base, std::uint8_t disturbance, std::uint64_t tick,
                     std::size_t index, std::uint32_t salt) noexcept {
  const std::uint32_t boosted = static_cast<std::uint32_t>(base) + disturbance / 3U;
  const std::uint32_t effective = std::min<std::uint32_t>(255U, boosted);
  if (effective == 0U) {
    return false;
  }
  if (effective == 255U) {
    return true;
  }

  const std::uint32_t phase = static_cast<std::uint32_t>(
      (tick * 37U + static_cast<std::uint64_t>(index) * 53U + salt * 97U) & 0xFFU);
  return phase < effective;
}

std::size_t ordered_index(std::size_t step, const Direction& gravity) noexcept {
  std::size_t x = step % kWorldWidth;
  std::size_t y = step / kWorldWidth;

  if (gravity.dx > 0) {
    x = kWorldWidth - 1U - x;
  }
  if (gravity.dy > 0) {
    y = kWorldHeight - 1U - y;
  }
  return y * kWorldWidth + x;
}

bool can_displace(const MaterialDynamicsInfo& source, const Cell& target,
                  const MaterialDynamicsInfo& target_info) noexcept {
  if (target.material == MaterialId::kEmpty || target.mass == 0U) {
    return true;
  }
  if (target_info.blocks_transport) {
    return false;
  }

  if (source.transport == TransportKind::kGravity) {
    return source.density > target_info.density;
  }
  if (source.transport == TransportKind::kBuoyant) {
    return source.density < target_info.density;
  }
  return false;
}

void set_motion(Cell& cell, int dx, int dy, std::uint8_t disturbance, bool opposite) noexcept {
  const int sign = opposite ? -1 : 1;
  const int magnitude = 48 + disturbance / 4U;
  cell.motion_x = static_cast<std::int8_t>(sign * dx * magnitude);
  cell.motion_y = static_cast<std::int8_t>(sign * dy * magnitude);
}

bool try_move(World& world, std::size_t source_index, int dx, int dy, std::uint8_t disturbance,
              bool lateral, std::array<bool, kWorldCapacity>& claimed,
              DynamicsStats& stats) noexcept {
  if (claimed[source_index]) {
    return false;
  }

  const int source_x = static_cast<int>(source_index % kWorldWidth);
  const int source_y = static_cast<int>(source_index / kWorldWidth);
  const int target_x = source_x + dx;
  const int target_y = source_y + dy;
  if (!world.in_bounds(target_x, target_y)) {
    return false;
  }

  const std::size_t target_index = static_cast<std::size_t>(target_y) * kWorldWidth +
                                   static_cast<std::size_t>(target_x);
  if (claimed[target_index]) {
    return false;
  }

  Cell* source_cell = world.try_cell_index(source_index);
  Cell* target_cell = world.try_cell_index(target_index);
  if (source_cell == nullptr || target_cell == nullptr || source_cell->mass == 0U) {
    return false;
  }

  const MaterialDynamicsInfo* source_info = material_dynamics_info(source_cell->material);
  const MaterialDynamicsInfo* target_info = material_dynamics_info(target_cell->material);
  if (source_info == nullptr || target_info == nullptr ||
      !can_displace(*source_info, *target_cell, *target_info)) {
    return false;
  }

  ++stats.transport_attempts;
  const bool swapped = target_cell->material != MaterialId::kEmpty && target_cell->mass != 0U;
  const Cell displaced = *target_cell;
  *target_cell = *source_cell;
  *source_cell = swapped ? displaced : Cell{};

  set_motion(*target_cell, dx, dy, disturbance, false);
  if (swapped) {
    set_motion(*source_cell, dx, dy, disturbance, true);
    ++stats.density_swaps;
  }

  claimed[source_index] = true;
  claimed[target_index] = true;
  ++stats.transport_moves;
  if (lateral) {
    ++stats.lateral_moves;
  }
  if (source_info->transport == TransportKind::kBuoyant) {
    ++stats.gas_moves;
  }
  return true;
}

void transport(World& world, const InputFrame& frame, std::uint64_t tick, DynamicsStats& stats) noexcept {
  const Direction gravity = choose_gravity_direction(frame, tick);
  stats.gravity_dx = static_cast<std::int8_t>(gravity.dx);
  stats.gravity_dy = static_cast<std::int8_t>(gravity.dy);
  stats.disturbance_q8 = disturbance_strength(frame);

  for (std::size_t index = 0; index < kWorldCapacity; ++index) {
    Cell* cell = world.try_cell_index(index);
    if (cell != nullptr) {
      cell->motion_x = decay_motion(cell->motion_x);
      cell->motion_y = decay_motion(cell->motion_y);
    }
  }

  if (!gravity.active) {
    return;
  }

  std::array<bool, kWorldCapacity> claimed{};
  for (std::size_t step = 0; step < kWorldCapacity; ++step) {
    const std::size_t index = ordered_index(step, gravity);
    if (claimed[index]) {
      continue;
    }

    Cell* cell = world.try_cell_index(index);
    if (cell == nullptr || cell->material == MaterialId::kEmpty || cell->mass == 0U) {
      continue;
    }

    const MaterialDynamicsInfo* info = material_dynamics_info(cell->material);
    if (info == nullptr || info->transport == TransportKind::kStatic || info->blocks_transport) {
      continue;
    }

    if (!mobility_allows(info->gravity_mobility, stats.disturbance_q8, tick, index, 1U)) {
      continue;
    }

    const int direction_sign = info->transport == TransportKind::kBuoyant ? -1 : 1;
    const int primary_dx = gravity.dx * direction_sign;
    const int primary_dy = gravity.dy * direction_sign;
    if (try_move(world, index, primary_dx, primary_dy, stats.disturbance_q8, false, claimed,
                 stats)) {
      continue;
    }

    if (!mobility_allows(info->lateral_mobility, stats.disturbance_q8, tick, index, 2U)) {
      continue;
    }

    const int clockwise_dx = -primary_dy;
    const int clockwise_dy = primary_dx;
    const bool prefer_clockwise =
        frame.spin_rate > 0.15F || (frame.spin_rate >= -0.15F && ((tick + index) & 1U) == 0U);
    const int first_dx = prefer_clockwise ? clockwise_dx : -clockwise_dx;
    const int first_dy = prefer_clockwise ? clockwise_dy : -clockwise_dy;
    if (try_move(world, index, first_dx, first_dy, stats.disturbance_q8, true, claimed, stats)) {
      continue;
    }
    static_cast<void>(try_move(world, index, -first_dx, -first_dy, stats.disturbance_q8, true,
                               claimed, stats));
  }
}

void update_fire_lifecycle(World& world, DynamicsStats& stats) noexcept {
  for (std::size_t index = 0; index < kWorldCapacity; ++index) {
    Cell* cell = world.try_cell_index(index);
    if (cell == nullptr || cell->material != MaterialId::kFire) {
      continue;
    }

    if (cell->aux == 0U) {
      cell->aux = 8U;
      cell->temperature = std::max<std::int16_t>(cell->temperature, 900);
      continue;
    }

    --cell->aux;
    if (cell->aux == 0U) {
      cell->material = MaterialId::kSmoke;
      cell->temperature = static_cast<std::int16_t>(cell->temperature / 2);
      ++stats.fire_cells_expired;
    }
  }
}

const ReactionRule* find_reaction(MaterialId first, MaterialId second, bool& reversed) noexcept {
  for (const ReactionRule& rule : kReactionRules) {
    if (rule.first == first && rule.second == second) {
      reversed = false;
      return &rule;
    }
    if (rule.first == second && rule.second == first) {
      reversed = true;
      return &rule;
    }
  }
  return nullptr;
}

void apply_product(Cell& cell, MaterialId material, std::int16_t minimum_temperature,
                   std::uint8_t minimum_aux) noexcept {
  cell.material = material;
  if (cell.temperature < minimum_temperature) {
    cell.temperature = minimum_temperature;
  }
  cell.aux = std::max(cell.aux, minimum_aux);
}

void apply_reaction(World& world, std::size_t first_index, std::size_t second_index,
                    const ReactionRule& rule, bool reversed, WorkBudget& event_budget,
                    DynamicsStats& stats) noexcept {
  Cell* first = world.try_cell_index(first_index);
  Cell* second = world.try_cell_index(second_index);
  if (first == nullptr || second == nullptr) {
    return;
  }

  Cell* rule_first = reversed ? second : first;
  Cell* rule_second = reversed ? first : second;
  apply_product(*rule_first, rule.first_product, rule.first_temperature, rule.first_aux);
  apply_product(*rule_second, rule.second_product, rule.second_temperature, rule.second_aux);

  ++stats.reactions_applied;
  if (!event_budget.try_consume()) {
    return;
  }

  const int first_x = static_cast<int>(first_index % kWorldWidth);
  const int first_y = static_cast<int>(first_index / kWorldWidth);
  const int second_x = static_cast<int>(second_index % kWorldWidth);
  const int second_y = static_cast<int>(second_index / kWorldWidth);
  const int dx = second_x - first_x;
  const int dy = second_y - first_y;
  set_motion(*first, dx, dy, 64U, true);
  set_motion(*second, dx, dy, 64U, false);
  ++stats.reaction_impulses;
}

void react(World& world, WorkBudget& event_budget, WorkBudget& reaction_budget,
           DynamicsStats& stats) noexcept {
  std::array<bool, kWorldCapacity> reacted{};
  for (std::size_t y = 0; y < kWorldHeight; ++y) {
    for (std::size_t x = 0; x < kWorldWidth; ++x) {
      const std::size_t first_index = y * kWorldWidth + x;
      if (reacted[first_index]) {
        continue;
      }

      constexpr int kNeighborOffsets[2][2] = {{1, 0}, {0, 1}};
      for (const auto& offset : kNeighborOffsets) {
        const int neighbor_x = static_cast<int>(x) + offset[0];
        const int neighbor_y = static_cast<int>(y) + offset[1];
        if (!world.in_bounds(neighbor_x, neighbor_y)) {
          continue;
        }

        const std::size_t second_index = static_cast<std::size_t>(neighbor_y) * kWorldWidth +
                                         static_cast<std::size_t>(neighbor_x);
        if (reacted[second_index]) {
          continue;
        }

        const Cell* first = world.try_cell_index(first_index);
        const Cell* second = world.try_cell_index(second_index);
        if (first == nullptr || second == nullptr) {
          continue;
        }

        bool reversed = false;
        const ReactionRule* rule = find_reaction(first->material, second->material, reversed);
        if (rule == nullptr) {
          continue;
        }

        ++stats.reaction_candidates;
        if (!reaction_budget.try_consume()) {
          continue;
        }

        apply_reaction(world, first_index, second_index, *rule, reversed, event_budget, stats);
        reacted[first_index] = true;
        reacted[second_index] = true;
        break;
      }
    }
  }
}

void exchange_heat(World& world, DynamicsStats& stats) noexcept {
  std::array<std::int32_t, kWorldCapacity> deltas{};

  for (std::size_t y = 0; y < kWorldHeight; ++y) {
    for (std::size_t x = 0; x < kWorldWidth; ++x) {
      const std::size_t first_index = y * kWorldWidth + x;
      constexpr int kNeighborOffsets[2][2] = {{1, 0}, {0, 1}};
      for (const auto& offset : kNeighborOffsets) {
        const int neighbor_x = static_cast<int>(x) + offset[0];
        const int neighbor_y = static_cast<int>(y) + offset[1];
        if (!world.in_bounds(neighbor_x, neighbor_y)) {
          continue;
        }

        const std::size_t second_index = static_cast<std::size_t>(neighbor_y) * kWorldWidth +
                                         static_cast<std::size_t>(neighbor_x);
        const Cell* first = world.try_cell_index(first_index);
        const Cell* second = world.try_cell_index(second_index);
        if (first == nullptr || second == nullptr) {
          continue;
        }

        const MaterialDynamicsInfo* first_info = material_dynamics_info(first->material);
        const MaterialDynamicsInfo* second_info = material_dynamics_info(second->material);
        if (first_info == nullptr || second_info == nullptr) {
          continue;
        }

        const std::int32_t difference = static_cast<std::int32_t>(second->temperature) -
                                        static_cast<std::int32_t>(first->temperature);
        const std::uint32_t conductivity =
            std::min(first_info->thermal_conductivity, second_info->thermal_conductivity);
        if (difference == 0 || conductivity == 0U) {
          continue;
        }

        std::int32_t exchange =
            (difference * static_cast<std::int32_t>(conductivity)) / 1024;
        exchange = std::clamp<std::int32_t>(exchange, -128, 128);
        if (exchange == 0) {
          continue;
        }

        deltas[first_index] += exchange;
        deltas[second_index] -= exchange;
        ++stats.heat_pairs;
      }
    }
  }

  for (std::size_t index = 0; index < kWorldCapacity; ++index) {
    Cell* cell = world.try_cell_index(index);
    if (cell == nullptr) {
      continue;
    }

    const MaterialDynamicsInfo* info = material_dynamics_info(cell->material);
    if (info == nullptr) {
      continue;
    }

    std::int32_t temperature = static_cast<std::int32_t>(cell->temperature) + deltas[index];
    const std::int32_t loss = info->ambient_loss;
    if (temperature > 0) {
      temperature = std::max<std::int32_t>(0, temperature - loss);
    } else if (temperature < 0) {
      temperature = std::min<std::int32_t>(0, temperature + loss);
    }
    cell->temperature = clamp_temperature(temperature);
  }
}

} // namespace

DynamicsStats DynamicsEngine::step(World& world, const InputFrame& input, std::uint64_t tick,
                                   WorkBudget& event_budget,
                                   WorkBudget& reaction_budget) const noexcept {
  const InputFrame frame = sanitize_input_frame(input);
  DynamicsStats stats{};
  transport(world, frame, tick, stats);
  update_fire_lifecycle(world, stats);
  react(world, event_budget, reaction_budget, stats);
  exchange_heat(world, stats);
  return stats;
}

} // namespace espsand::sim
