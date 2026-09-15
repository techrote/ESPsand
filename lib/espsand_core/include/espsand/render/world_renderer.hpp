#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::render {

enum class RenderMode : std::uint8_t {
  kBeauty = 0,
  kMaterialId,
  kTemperature,
  kMass,
};

struct RenderConfig {
  RenderMode mode = RenderMode::kBeauty;
  std::uint16_t exposure_q8 = 256;
};

struct RenderStats {
  std::uint16_t unknown_material_cells = 0;
  std::uint16_t minority_preserved_pixels = 0;
};

struct RenderResult {
  io::Frame8x8 frame{};
  RenderStats stats{};
};

class WorldRenderer {
public:
  RenderResult render(const sim::World& world, const RenderConfig& config = {}) const noexcept;
};

} // namespace espsand::render
