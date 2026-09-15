#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::render {

inline constexpr std::uint8_t kOrdinaryDisplayCenter = 63U;
inline constexpr std::uint8_t kOrdinaryDisplayCeiling = 127U;
inline constexpr std::uint8_t kPseudoHdrFloor = 128U;

// Beauty-mode values 0..127 are the ordinary perceptual domain. 128..255 is intentionally
// reserved for sparse energetic/reactive/highlight state. These are LED code values, not calibrated
// luminance, current or thermal ratings.

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
  std::uint16_t pseudo_hdr_pixels = 0;
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
