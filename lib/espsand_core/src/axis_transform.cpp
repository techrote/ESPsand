#include <espsand/input/axis_transform.hpp>

namespace espsand::input {

float component(const io::Vec3& value, Axis axis) {
  switch (axis) {
  case Axis::kX:
    return value.x;
  case Axis::kY:
    return value.y;
  case Axis::kZ:
    return value.z;
  }
  return 0.0F;
}

io::Vec2 project_to_matrix(const io::Vec3& value, const PlaneTransform& transform) {
  return {
      component(value, transform.matrix_x.axis) * static_cast<float>(transform.matrix_x.sign),
      component(value, transform.matrix_y.axis) * static_cast<float>(transform.matrix_y.sign),
  };
}

} // namespace espsand::input
