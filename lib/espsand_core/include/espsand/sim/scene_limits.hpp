#pragma once

#include <cstdint>

#include <espsand/sim/materials.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::sim {

// Product scenes intentionally keep every individual material below 16 logical cells.
// This is stricter than the physical requirement because one logical cell can affect at most
// one physical 2x2 aggregate, so the same material can never occupy 16 or more LEDs solely
// through scene-owned creation/growth.
inline constexpr std::uint16_t kProductMaterialCellLimit = 15U;

inline std::uint16_t material_cell_count(const World& world, MaterialId material) noexcept {
  if (!is_valid_material(material)) {
    return 0U;
  }
  return world.totals().cell_count[material_index(material)];
}

inline bool can_add_product_material(const World& world, MaterialId material,
                                     std::uint16_t count = 1U) noexcept {
  const std::uint32_t current = material_cell_count(world, material);
  return current + count <= kProductMaterialCellLimit;
}

} // namespace espsand::sim
