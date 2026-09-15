#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>
#include <espsand/sim/moss_garden_scene.hpp>
#include <espsand/sim/world.hpp>

namespace espsand::render {

void apply_structural_contrast(io::Frame8x8& frame, const sim::World& world) noexcept;

void blend_with_previous(io::Frame8x8& frame, const io::Frame8x8& previous,
                         std::uint8_t previous_weight_q8) noexcept;

void apply_mite_overlay(io::Frame8x8& frame, const sim::MossGardenStateSnapshot& state) noexcept;

} // namespace espsand::render
