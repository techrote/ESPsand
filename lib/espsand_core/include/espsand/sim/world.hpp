#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <espsand/sim/materials.hpp>

namespace espsand::sim {

inline constexpr std::uint32_t kCellSchemaVersion = 1;
inline constexpr std::size_t kWorldWidth = 16;
inline constexpr std::size_t kWorldHeight = 16;
inline constexpr std::size_t kWorldCapacity = kWorldWidth * kWorldHeight;

struct Cell {
  MaterialId material = MaterialId::kEmpty;
  std::uint8_t mass = 0;
  std::int8_t motion_x = 0;
  std::int8_t motion_y = 0;
  std::int16_t temperature = 0;
  std::uint8_t aux = 0;
  std::uint8_t flags = 0;
};

static_assert(sizeof(Cell) == 8, "Cell must remain compact and fixed-size");
static_assert(std::is_trivially_copyable_v<Cell>, "Cell must remain a simple value type");

struct MaterialTotals {
  std::array<std::uint16_t, kMaterialCount> cell_count{};
  std::array<std::uint32_t, kMaterialCount> mass{};
  std::uint16_t occupied_cells = 0;
  std::uint16_t invalid_cells = 0;
  std::uint32_t total_mass = 0;
  std::uint32_t invalid_mass = 0;
};

class World {
public:
  World() noexcept;

  void clear() noexcept;
  bool in_bounds(int x, int y) const noexcept;

  Cell* try_cell(int x, int y) noexcept;
  const Cell* try_cell(int x, int y) const noexcept;

  Cell* try_cell_index(std::size_t index) noexcept;
  const Cell* try_cell_index(std::size_t index) const noexcept;

  bool set_cell(int x, int y, const Cell& cell) noexcept;

  MaterialTotals totals() const noexcept;
  bool invariants_hold() const noexcept;

  const std::array<Cell, kWorldCapacity>& cells() const noexcept {
    return cells_;
  }

private:
  static constexpr std::size_t index_unchecked(std::size_t x, std::size_t y) noexcept {
    return y * kWorldWidth + x;
  }

  std::array<Cell, kWorldCapacity> cells_{};
};

} // namespace espsand::sim
