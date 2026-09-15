#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/work_budget.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kDynamicsSchemaVersion = 1;

enum class TransportKind : std::uint8_t {
  kStatic = 0,
  kGravity,
  kBuoyant,
};

struct MaterialDynamicsInfo {
  MaterialId id = MaterialId::kEmpty;
  TransportKind transport = TransportKind::kStatic;
  std::uint8_t density = 0;
  std::uint8_t gravity_mobility = 0;
  std::uint8_t lateral_mobility = 0;
  std::uint8_t thermal_conductivity = 0;
  std::uint8_t ambient_loss = 0;
  bool blocks_transport = false;
};

inline constexpr std::array<MaterialDynamicsInfo, kMaterialCount> kMaterialDynamics{{
    {MaterialId::kEmpty, TransportKind::kStatic, 0, 0, 0, 12, 4, false},
    {MaterialId::kWall, TransportKind::kStatic, 255, 0, 0, 20, 1, true},
    {MaterialId::kCrust, TransportKind::kStatic, 240, 0, 0, 28, 1, true},
    {MaterialId::kWater, TransportKind::kGravity, 120, 255, 224, 56, 2, false},
    {MaterialId::kOil, TransportKind::kGravity, 80, 208, 176, 28, 2, false},
    {MaterialId::kLava, TransportKind::kGravity, 150, 112, 72, 72, 1, false},
    {MaterialId::kSteam, TransportKind::kBuoyant, 12, 255, 224, 20, 5, false},
    {MaterialId::kSmoke, TransportKind::kBuoyant, 8, 224, 208, 12, 6, false},
    {MaterialId::kFire, TransportKind::kBuoyant, 4, 248, 224, 8, 8, false},
    {MaterialId::kSodiumLike, TransportKind::kGravity, 180, 224, 104, 40, 2, false},
    {MaterialId::kTracer, TransportKind::kStatic, 96, 0, 0, 24, 3, false},
    {MaterialId::kMoss, TransportKind::kStatic, 160, 0, 0, 16, 2, true},
}};

constexpr const MaterialDynamicsInfo* material_dynamics_info(MaterialId id) noexcept {
  return is_valid_material(id) ? &kMaterialDynamics[material_index(id)] : nullptr;
}

struct ReactionRule {
  MaterialId first = MaterialId::kEmpty;
  MaterialId second = MaterialId::kEmpty;
  MaterialId first_product = MaterialId::kEmpty;
  MaterialId second_product = MaterialId::kEmpty;
  std::int16_t first_temperature = 0;
  std::int16_t second_temperature = 0;
  std::uint8_t first_aux = 0;
  std::uint8_t second_aux = 0;
};

// clang-format off
// Keep the reaction tuples compact so material/product/thermal changes remain directly reviewable.
inline constexpr ReactionRule kLavaWaterReaction{
    MaterialId::kLava, MaterialId::kWater, MaterialId::kCrust, MaterialId::kSteam, 700, 600, 0, 0};
inline constexpr ReactionRule kSodiumWaterReaction{
    MaterialId::kSodiumLike, MaterialId::kWater, MaterialId::kFire, MaterialId::kSteam, 1500, 900, 12, 0};
inline constexpr ReactionRule kOilFireReaction{
    MaterialId::kOil, MaterialId::kFire, MaterialId::kFire, MaterialId::kFire, 1100, 1000, 16, 12};
// clang-format on

inline constexpr std::array<ReactionRule, 3> kReactionRules{{
    kLavaWaterReaction,
    kSodiumWaterReaction,
    kOilFireReaction,
}};

struct DynamicsStats {
  std::uint16_t transport_attempts = 0;
  std::uint16_t transport_moves = 0;
  std::uint16_t density_swaps = 0;
  std::uint16_t lateral_moves = 0;
  std::uint16_t gas_moves = 0;
  std::uint16_t heat_pairs = 0;
  std::uint16_t reaction_candidates = 0;
  std::uint16_t reactions_applied = 0;
  std::uint16_t reaction_impulses = 0;
  std::uint16_t fire_cells_expired = 0;
  std::uint8_t disturbance_q8 = 0;
  std::int8_t gravity_dx = 0;
  std::int8_t gravity_dy = 0;
};

class DynamicsEngine {
public:
  DynamicsStats step(World& world, const InputFrame& input, std::uint64_t tick,
                     WorkBudget& event_budget, WorkBudget& reaction_budget) const noexcept;
};

} // namespace espsand::sim
