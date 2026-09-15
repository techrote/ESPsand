#include <espsand/sim/world.hpp>

namespace espsand::sim {

World::World() noexcept {
  clear();
}

void World::clear() noexcept {
  cells_.fill(Cell{});
}

bool World::in_bounds(int x, int y) const noexcept {
  return x >= 0 && y >= 0 && x < static_cast<int>(kWorldWidth) &&
         y < static_cast<int>(kWorldHeight);
}

Cell* World::try_cell(int x, int y) noexcept {
  if (!in_bounds(x, y)) {
    return nullptr;
  }

  return &cells_[index_unchecked(static_cast<std::size_t>(x), static_cast<std::size_t>(y))];
}

const Cell* World::try_cell(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return nullptr;
  }

  return &cells_[index_unchecked(static_cast<std::size_t>(x), static_cast<std::size_t>(y))];
}

Cell* World::try_cell_index(std::size_t index) noexcept {
  return index < cells_.size() ? &cells_[index] : nullptr;
}

const Cell* World::try_cell_index(std::size_t index) const noexcept {
  return index < cells_.size() ? &cells_[index] : nullptr;
}

bool World::set_cell(int x, int y, const Cell& cell) noexcept {
  Cell* target = try_cell(x, y);
  if (target == nullptr || !is_valid_material(cell.material)) {
    return false;
  }

  *target = cell;
  return true;
}

MaterialTotals World::totals() const noexcept {
  MaterialTotals totals{};

  for (const Cell& cell : cells_) {
    totals.total_mass += cell.mass;

    if (!is_valid_material(cell.material)) {
      ++totals.invalid_cells;
      totals.invalid_mass += cell.mass;
      continue;
    }

    const std::size_t index = material_index(cell.material);
    ++totals.cell_count[index];
    totals.mass[index] += cell.mass;

    if (cell.material != MaterialId::kEmpty) {
      ++totals.occupied_cells;
    }
  }

  return totals;
}

bool World::invariants_hold() const noexcept {
  return totals().invalid_cells == 0;
}

} // namespace espsand::sim
