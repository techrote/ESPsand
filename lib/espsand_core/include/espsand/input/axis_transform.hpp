#pragma once

#include <cstdint>

#include <espsand/io/types.hpp>

namespace espsand::input {

enum class Axis : std::uint8_t {
  kX,
  kY,
  kZ,
};

struct SignedAxis {
  Axis axis = Axis::kX;
  std::int8_t sign = 1;
};

struct PlaneTransform {
  SignedAxis matrix_x{};
  SignedAxis matrix_y{Axis::kY, 1};
};

float component(const io::Vec3& value, Axis axis);
io::Vec2 project_to_matrix(const io::Vec3& value, const PlaneTransform& transform);

} // namespace espsand::input
