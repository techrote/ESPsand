#include <espsand/render/scene_effects.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <espsand/sim/materials.hpp>

namespace espsand::render {
namespace {

struct BlockSignature {
  sim::MaterialId dominant = sim::MaterialId::kEmpty;
  std::uint8_t occupied_samples = 0;
  std::uint16_t motion = 0;
};

std::uint8_t blend_channel(std::uint8_t current, std::uint8_t previous,
                           std::uint8_t previous_weight_q8) noexcept {
  const std::uint32_t previous_weight = previous_weight_q8;
  const std::uint32_t current_weight = 255U - previous_weight;
  const std::uint32_t blended = static_cast<std::uint32_t>(current) * current_weight +
                                static_cast<std::uint32_t>(previous) * previous_weight;
  return static_cast<std::uint8_t>((blended + 127U) / 255U);
}

bool structural_material(sim::MaterialId material) noexcept {
  return material == sim::MaterialId::kWater || material == sim::MaterialId::kOil ||
         material == sim::MaterialId::kLava || material == sim::MaterialId::kMoss;
}

BlockSignature signature_for(const sim::World& world, std::size_t out_x,
                             std::size_t out_y) noexcept {
  std::array<std::uint16_t, sim::kMaterialCount> mass{};
  BlockSignature signature{};
  const std::size_t base_x = out_x * 2U;
  const std::size_t base_y = out_y * 2U;

  for (std::size_t local_y = 0; local_y < 2U; ++local_y) {
    for (std::size_t local_x = 0; local_x < 2U; ++local_x) {
      const sim::Cell* cell = world.try_cell(static_cast<int>(base_x + local_x),
                                             static_cast<int>(base_y + local_y));
      if (cell == nullptr || cell->material == sim::MaterialId::kEmpty || cell->mass == 0U ||
          !sim::is_valid_material(cell->material)) {
        continue;
      }
      ++signature.occupied_samples;
      mass[sim::material_index(cell->material)] = static_cast<std::uint16_t>(
          mass[sim::material_index(cell->material)] + cell->mass);
      signature.motion = static_cast<std::uint16_t>(
          signature.motion + std::abs(static_cast<int>(cell->motion_x)) +
          std::abs(static_cast<int>(cell->motion_y)));
    }
  }

  std::uint16_t dominant_mass = 0;
  for (std::size_t index = 1; index < sim::kMaterialCount; ++index) {
    if (mass[index] > dominant_mass) {
      dominant_mass = mass[index];
      signature.dominant = static_cast<sim::MaterialId>(index);
    }
  }
  return signature;
}

std::uint8_t same_material_neighbors(
    const std::array<BlockSignature, io::kMatrixPixels>& signatures, std::size_t x,
    std::size_t y) noexcept {
  constexpr std::array<std::array<int, 2>, 4> kDirections{{
      {{1, 0}},
      {{-1, 0}},
      {{0, 1}},
      {{0, -1}},
  }};
  const sim::MaterialId material = signatures[y * io::kMatrixWidth + x].dominant;
  std::uint8_t count = 0;
  for (const auto& direction : kDirections) {
    const int nx = static_cast<int>(x) + direction[0];
    const int ny = static_cast<int>(y) + direction[1];
    if (nx < 0 || ny < 0 || nx >= static_cast<int>(io::kMatrixWidth) ||
        ny >= static_cast<int>(io::kMatrixHeight)) {
      continue;
    }
    const auto& neighbor =
        signatures[static_cast<std::size_t>(ny) * io::kMatrixWidth + static_cast<std::size_t>(nx)];
    if (neighbor.dominant == material) {
      ++count;
    }
  }
  return count;
}

void scale_pixel(io::Rgb& pixel, std::uint16_t scale_q8) noexcept {
  const auto scale = [scale_q8](std::uint8_t channel) {
    const std::uint32_t value =
        (static_cast<std::uint32_t>(channel) * scale_q8 + 128U) / 256U;
    return static_cast<std::uint8_t>(std::min<std::uint32_t>(255U, value));
  };
  pixel.r = scale(pixel.r);
  pixel.g = scale(pixel.g);
  pixel.b = scale(pixel.b);
}

} // namespace

void apply_structural_contrast(io::Frame8x8& frame, const sim::World& world) noexcept {
  std::array<BlockSignature, io::kMatrixPixels> signatures{};
  for (std::size_t y = 0; y < io::kMatrixHeight; ++y) {
    for (std::size_t x = 0; x < io::kMatrixWidth; ++x) {
      signatures[y * io::kMatrixWidth + x] = signature_for(world, x, y);
    }
  }

  constexpr std::array<std::uint16_t, 4> kGrainScale{{238U, 248U, 258U, 268U}};
  for (std::size_t y = 0; y < io::kMatrixHeight; ++y) {
    for (std::size_t x = 0; x < io::kMatrixWidth; ++x) {
      const std::size_t index = y * io::kMatrixWidth + x;
      const BlockSignature& signature = signatures[index];
      if (!structural_material(signature.dominant) || signature.occupied_samples == 0U) {
        continue;
      }

      const std::size_t material_phase = sim::material_index(signature.dominant) * 7U;
      const std::size_t phase = (x * 3U + y * 5U + material_phase) % kGrainScale.size();
      const std::uint8_t same_neighbors = same_material_neighbors(signatures, x, y);
      const std::uint16_t edge_bonus = static_cast<std::uint16_t>((4U - same_neighbors) * 5U);
      const std::uint16_t motion_bonus = std::min<std::uint16_t>(18U, signature.motion / 3U);
      const std::uint16_t scale_q8 = static_cast<std::uint16_t>(
          std::min<std::uint16_t>(292U, kGrainScale[phase] + edge_bonus + motion_bonus));
      scale_pixel(frame[index], scale_q8);
    }
  }
}

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

void apply_mite_overlay(io::Frame8x8& frame, const sim::MossGardenStateSnapshot& state) noexcept {
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
