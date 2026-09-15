#include <espsand/input/motion_interpreter.hpp>

#include <algorithm>
#include <cmath>

namespace espsand::input {
namespace {

float finite_or_zero(float value) noexcept {
  return std::isfinite(value) ? value : 0.0F;
}

float clamp_unit(float value) noexcept {
  return std::clamp(value, 0.0F, 1.0F);
}

float magnitude(const io::Vec3& value) noexcept {
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

io::Vec3 sanitized(const io::Vec3& value) noexcept {
  return {finite_or_zero(value.x), finite_or_zero(value.y), finite_or_zero(value.z)};
}

io::Vec3 difference(const io::Vec3& first, const io::Vec3& second) noexcept {
  return {first.x - second.x, first.y - second.y, first.z - second.z};
}

float confidence_from_acceleration(float acceleration_magnitude) noexcept {
  const float error = std::fabs(acceleration_magnitude - 1.0F);
  return clamp_unit(1.0F - error / 0.75F);
}

} // namespace

void MotionInterpreter::reset() noexcept {
  gravity_estimate_ = {};
  snapshot_ = {};
  last_tap_us_ = 0;
  initialized_ = false;
}

void MotionInterpreter::update(const io::ImuSample& sample,
                               const PlaneTransform& transform) noexcept {
  if (!sample.valid) {
    snapshot_.shake_energy *= 0.78F;
    snapshot_.motion_energy *= 0.88F;
    snapshot_.tap_impulse = 0.0F;
    snapshot_.spin_rate *= 0.8F;
    return;
  }

  const io::Vec3 acceleration = sanitized(sample.accel_g);
  const io::Vec3 gyro = sanitized(sample.gyro_dps);
  const float acceleration_magnitude = magnitude(acceleration);

  if (!initialized_) {
    gravity_estimate_ = acceleration;
    initialized_ = true;
    snapshot_.ready = true;
  } else {
    const io::Vec3 residual = difference(acceleration, gravity_estimate_);
    const float residual_magnitude = magnitude(residual);
    const float confidence = confidence_from_acceleration(acceleration_magnitude);

    const float gravity_alpha = 0.01F + confidence * 0.07F;
    gravity_estimate_.x += (acceleration.x - gravity_estimate_.x) * gravity_alpha;
    gravity_estimate_.y += (acceleration.y - gravity_estimate_.y) * gravity_alpha;
    gravity_estimate_.z += (acceleration.z - gravity_estimate_.z) * gravity_alpha;

    const float instant_shake = clamp_unit((residual_magnitude - 0.08F) / 0.70F);
    snapshot_.shake_energy = std::max(snapshot_.shake_energy * 0.78F, instant_shake);

    const float gyro_magnitude = magnitude(gyro);
    const float acceleration_motion = clamp_unit(residual_magnitude / 0.70F);
    const float rotation_motion = clamp_unit(gyro_magnitude / 360.0F);
    const float instant_motion = std::max(acceleration_motion, rotation_motion);
    snapshot_.motion_energy = std::max(snapshot_.motion_energy * 0.88F, instant_motion);

    snapshot_.tap_impulse = 0.0F;
    const bool cooldown_elapsed =
        last_tap_us_ == 0U || sample.timestamp_us - last_tap_us_ >= 160000U;
    if (residual_magnitude >= 0.72F && cooldown_elapsed) {
      snapshot_.tap_impulse =
          clamp_unit(0.35F + (residual_magnitude - 0.72F) / 0.80F);
      last_tap_us_ = sample.timestamp_us;
    }
  }

  const io::Vec2 projected = project_to_matrix(gravity_estimate_, transform);
  snapshot_.gravity.x = std::clamp(finite_or_zero(projected.x), -1.0F, 1.0F);
  snapshot_.gravity.y = std::clamp(finite_or_zero(projected.y), -1.0F, 1.0F);
  snapshot_.gravity_magnitude = clamp_unit(std::sqrt(snapshot_.gravity.x * snapshot_.gravity.x +
                                                     snapshot_.gravity.y * snapshot_.gravity.y));
  snapshot_.gravity_confidence = confidence_from_acceleration(acceleration_magnitude);
  snapshot_.spin_rate = std::clamp(gyro.z / 360.0F, -1.0F, 1.0F);
  snapshot_.ready = true;
}

MotionSnapshot MotionInterpreter::snapshot() const noexcept {
  return snapshot_;
}

void MotionInterpreter::consume_transients() noexcept {
  snapshot_.tap_impulse = 0.0F;
}

} // namespace espsand::input
