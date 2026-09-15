#include <espsand/render/scene_effects.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace espsand::render {
namespace {

std::uint8_t blend_channel(std::uint8_t current, std::uint8_t previous,
                           std::uint8_t previous_weight_q8) noexcept {
  const std::uint32_t previous_weight = previous_weight_q8;
  const std::uint32_t current_weight = 255U - previous_weight;
  const std::uint32_t blended = static_cast<std::uint32_t>(current) * current_weight +
                                static_cast<std::uint32_t>(previous) * previous_weight;
  return static_cast<std::uint8_t>((blended + 127U) / 255U);
}

} // namespace

void blend_with_previous(io::Frame8x8& frame, const io::Frame8x8& previous,
                         std::uint8_t previous_weight_q8) noexcept {
  if (previous_weight_q8 == 0U) {
    return;
  }
  for (std::size_t index = 0; index < io::kMatrixPixels; ++index) {
    frame[index].r = blend_channel(frame[index].r, previous[index].r, previous_weight_q8);
    frame[index].g = blend_channel(frame[index].g, previous[index].g, previous_weight_q8);
    frame[index].b = blend_channel(frame[index].b, previous[index].b, previous_weight_q8);
  }
}

void apply_mite_overlay(io::Frame8x8& frame,
                        const sim::MossGardenStateSnapshot& state) noexcept {
  for (const sim::MiteState& mite : state.mites) {
    if (!mite.active) {
      continue;
    }
    const std::size_t output_x = mite.x / 2U;
    const std::size_t output_y = mite.y / 2U;
    if (output_x >= io::kMatrixWidth || output_y >= io::kMatrixHeight) {
      continue;
    }
    io::Rgb& pixel = frame[output_y * io::kMatrixWidth + output_x];
    const std::uint8_t energy = mite.energy;
    pixel.r = std::max<std::uint8_t>(pixel.r, static_cast<std::uint8_t>(190U + energy / 4U));
    pixel.g = std::max<std::uint8_t>(pixel.g, static_cast<std::uint8_t>(52U + energy / 6U));
    pixel.b = std::max<std::uint8_t>(pixel.b, 245U);
  }
}

} // namespace espsand::render
