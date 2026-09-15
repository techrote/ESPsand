#include <espsand/render/world_renderer.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <espsand/sim/materials.hpp>

namespace espsand::render {
namespace {

struct LinearRgb {
  std::uint32_t r = 0;
  std::uint32_t g = 0;
  std::uint32_t b = 0;
};

struct MaterialStyle {
  LinearRgb base{};
  std::uint8_t priority = 0;
};

constexpr std::array<MaterialStyle, sim::kMaterialCount> kStyles{{
    {{0, 0, 0}, 0},
    {{520, 520, 580}, 48},
    {{240, 70, 25}, 112},
    {{50, 520, 1700}, 72},
    {{760, 360, 70}, 64},
    {{3200, 780, 70}, 232},
    {{1050, 1500, 1900}, 192},
    {{280, 230, 360}, 104},
    {{3800, 1900, 260}, 255},
    {{1900, 1550, 620}, 216},
    {{1350, 2100, 260}, 200},
    {{160, 1450, 180}, 176},
}};

constexpr MaterialStyle kUnknownStyle{{380, 0, 380}, 80};
constexpr std::uint32_t kToneKnee = 1024U;

const MaterialStyle& style_for(sim::MaterialId material, bool& valid) noexcept {
  valid = sim::is_valid_material(material);
  if (!valid) {
    return kUnknownStyle;
  }
  return kStyles[sim::material_index(material)];
}

LinearRgb shade_cell(const sim::Cell& cell, const MaterialStyle& style) noexcept {
  LinearRgb color = style.base;

  if (cell.material == sim::MaterialId::kTracer) {
    const std::uint32_t concentration = cell.aux;
    color.r += concentration * 4U;
    color.g += concentration * 2U;
    color.b = color.b > concentration ? color.b - concentration : 0U;
  }

  if (cell.material == sim::MaterialId::kMoss) {
    color.g += static_cast<std::uint32_t>(cell.aux) * 3U;
  }

  const std::uint32_t heat =
      cell.temperature > 0 ? std::min<std::uint32_t>(cell.temperature, 2048U) : 0U;
  color.r += heat;
  color.g += heat / 2U;
  color.b += heat / 8U;
  return color;
}

std::uint8_t importance_for(const sim::Cell& cell, const MaterialStyle& style) noexcept {
  const std::uint32_t heat =
      cell.temperature > 0 ? std::min<std::uint32_t>(cell.temperature / 16, 63U) : 0U;
  const std::uint32_t aux_bonus =
      (cell.material == sim::MaterialId::kTracer || cell.material == sim::MaterialId::kMoss)
          ? cell.aux / 8U
          : 0U;
  return static_cast<std::uint8_t>(
      std::min<std::uint32_t>(255U, static_cast<std::uint32_t>(style.priority) + heat + aux_bonus));
}

std::uint8_t accent_weight(std::uint8_t importance) noexcept {
  if (importance >= 240U) {
    return 192U;
  }
  if (importance >= 210U) {
    return 160U;
  }
  if (importance >= 175U) {
    return 128U;
  }
  if (importance >= 145U) {
    return 96U;
  }
  return 0U;
}

std::uint32_t lerp_channel(std::uint32_t base, std::uint32_t accent, std::uint8_t weight) noexcept {
  const std::uint32_t inverse = 255U - weight;
  return (base * inverse + accent * weight + 127U) / 255U;
}

void scale_color_q8(LinearRgb& color, std::uint16_t scale_q8) noexcept {
  color.r = (color.r * scale_q8 + 128U) / 256U;
  color.g = (color.g * scale_q8 + 128U) / 256U;
  color.b = (color.b * scale_q8 + 128U) / 256U;
}

std::uint16_t coverage_scale_q8(std::uint8_t occupied_samples) noexcept {
  // A partially occupied logical 2x2 block must not project as a fully filled physical LED.
  // Keep thin one-cell structures visible while preserving a clear difference between 1/4 and 4/4 fill.
  constexpr std::array<std::uint16_t, 5> kCoverageScale{{0U, 136U, 176U, 216U, 256U}};
  return kCoverageScale[std::min<std::size_t>(occupied_samples, 4U)];
}

std::uint8_t tone_map(std::uint32_t linear, std::uint16_t exposure_q8) noexcept {
  const std::uint64_t scaled = (static_cast<std::uint64_t>(linear) * exposure_q8 + 128U) / 256U;
  if (scaled == 0U) {
    return 0;
  }
  const std::uint64_t mapped = (scaled * 255U) / (scaled + kToneKnee);
  return static_cast<std::uint8_t>(std::min<std::uint64_t>(255U, mapped));
}

io::Rgb tone_map(const LinearRgb& color, std::uint16_t exposure_q8) noexcept {
  return {tone_map(color.r, exposure_q8), tone_map(color.g, exposure_q8),
          tone_map(color.b, exposure_q8)};
}

std::uint32_t temperature_magnitude(std::int16_t temperature) noexcept {
  const std::int32_t value = temperature;
  return static_cast<std::uint32_t>(value < 0 ? -value : value);
}

LinearRgb temperature_color(std::int16_t temperature) noexcept {
  if (temperature == 0) {
    return {};
  }
  if (temperature < 0) {
    const std::uint32_t magnitude =
        std::min<std::uint32_t>(temperature_magnitude(temperature), 2048U);
    return {0, magnitude / 3U, magnitude + 400U};
  }
  const std::uint32_t heat = std::min<std::uint32_t>(temperature, 2048U);
  return {heat + 800U, heat / 2U + 120U, heat / 12U};
}

} // namespace

RenderResult WorldRenderer::render(const sim::World& world,
                                   const RenderConfig& config) const noexcept {
  RenderResult result{};

  for (std::size_t out_y = 0; out_y < io::kMatrixHeight; ++out_y) {
    for (std::size_t out_x = 0; out_x < io::kMatrixWidth; ++out_x) {
      const std::size_t base_x = out_x * 2U;
      const std::size_t base_y = out_y * 2U;
      LinearRgb aggregate{};
      std::uint32_t total_mass = 0;
      LinearRgb accent{};
      std::uint8_t accent_importance = 0;
      std::uint32_t accent_mass = 0;
      const sim::Cell* diagnostic_cell = nullptr;
      std::uint8_t diagnostic_mass = 0;
      std::int16_t diagnostic_temperature = 0;
      std::uint32_t block_mass = 0;
      std::uint8_t occupied_samples = 0;

      for (std::size_t local_y = 0; local_y < 2U; ++local_y) {
        for (std::size_t local_x = 0; local_x < 2U; ++local_x) {
          const sim::Cell* cell = world.try_cell(static_cast<int>(base_x + local_x),
                                                 static_cast<int>(base_y + local_y));
          if (cell == nullptr) {
            continue;
          }

          bool valid = false;
          const MaterialStyle& style = style_for(cell->material, valid);
          if (!valid) {
            ++result.stats.unknown_material_cells;
          }

          block_mass += cell->mass;
          if (diagnostic_cell == nullptr || cell->mass > diagnostic_mass) {
            diagnostic_cell = cell;
            diagnostic_mass = cell->mass;
          }
          if (temperature_magnitude(cell->temperature) >
              temperature_magnitude(diagnostic_temperature)) {
            diagnostic_temperature = cell->temperature;
          }

          if (cell->material == sim::MaterialId::kEmpty || cell->mass == 0U) {
            continue;
          }

          ++occupied_samples;
          const LinearRgb shaded = shade_cell(*cell, style);
          aggregate.r += shaded.r * cell->mass;
          aggregate.g += shaded.g * cell->mass;
          aggregate.b += shaded.b * cell->mass;
          total_mass += cell->mass;

          const std::uint8_t importance = importance_for(*cell, style);
          if (importance > accent_importance) {
            accent_importance = importance;
            accent = shaded;
            accent_mass = cell->mass;
          }
        }
      }

      const std::size_t output_index = out_y * io::kMatrixWidth + out_x;
      if (config.mode == RenderMode::kMaterialId) {
        if (diagnostic_cell != nullptr) {
          bool valid = false;
          const MaterialStyle& style = style_for(diagnostic_cell->material, valid);
          result.frame[output_index] = tone_map(style.base, config.exposure_q8);
        }
        continue;
      }

      if (config.mode == RenderMode::kTemperature) {
        result.frame[output_index] =
            tone_map(temperature_color(diagnostic_temperature), config.exposure_q8);
        continue;
      }

      if (config.mode == RenderMode::kMass) {
        const std::uint32_t average_mass = block_mass / 4U;
        const LinearRgb mass_color{average_mass * 3U, average_mass * 5U, average_mass * 6U};
        result.frame[output_index] = tone_map(mass_color, config.exposure_q8);
        continue;
      }

      if (total_mass == 0U) {
        continue;
      }

      aggregate.r /= total_mass;
      aggregate.g /= total_mass;
      aggregate.b /= total_mass;
      scale_color_q8(aggregate, coverage_scale_q8(occupied_samples));

      const std::uint8_t weight = accent_weight(accent_importance);
      if (weight != 0U) {
        if (accent_mass * 2U < total_mass) {
          ++result.stats.minority_preserved_pixels;
        }
        aggregate.r = lerp_channel(aggregate.r, accent.r, weight);
        aggregate.g = lerp_channel(aggregate.g, accent.g, weight);
        aggregate.b = lerp_channel(aggregate.b, accent.b, weight);
      }

      result.frame[output_index] = tone_map(aggregate, config.exposure_q8);
    }
  }

  return result;
}

} // namespace espsand::render
