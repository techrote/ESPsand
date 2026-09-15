#pragma once

#include <cstdint>

#include <espsand/input/axis_transform.hpp>
#include <espsand/io/types.hpp>
#include <espsand/sim/input_frame.hpp>

namespace espsand::input {

struct MotionSnapshot {
  sim::NormalizedVec2 gravity{};
  float gravity_magnitude = 0.0F;
  float gravity_confidence = 0.0F;
  float shake_energy = 0.0F;
  float motion_energy = 0.0F;
  float tap_impulse = 0.0F;
  float spin_rate = 0.0F;
  bool ready = false;
};

class MotionInterpreter {
public:
  void reset() noexcept;
  void update(const io::ImuSample& sample, const PlaneTransform& transform) noexcept;
  MotionSnapshot snapshot() const noexcept;
  void consume_transients() noexcept;

private:
  io::Vec3 gravity_estimate_{};
  MotionSnapshot snapshot_{};
  std::uint64_t last_tap_us_ = 0;
  bool initialized_ = false;
};

} // namespace espsand::input
