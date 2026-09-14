#include "diagnostic_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "board/board_profile.hpp"
#include "board/diagnostics.hpp"

namespace espsand::app {
namespace {

constexpr std::uint64_t kImuPeriodUs = 5000;
constexpr std::uint64_t kRenderPeriodUs = 16667;
constexpr std::uint64_t kTelemetryPeriodUs = 1000000;
constexpr std::uint8_t kNormalBrightness = 24;
constexpr std::uint8_t kPrimaryBrightness = 16;

std::size_t pixel_index(int x, int y) {
  const int clamped_x = std::clamp(x, 0, static_cast<int>(io::kMatrixWidth) - 1);
  const int clamped_y = std::clamp(y, 0, static_cast<int>(io::kMatrixHeight) - 1);
  return static_cast<std::size_t>(clamped_y) * io::kMatrixWidth +
         static_cast<std::size_t>(clamped_x);
}

const char* event_name(io::ButtonEvent event) {
  switch (event) {
  case io::ButtonEvent::kNone:
    return "none";
  case io::ButtonEvent::kShortPress:
    return "short";
  case io::ButtonEvent::kLongPress:
    return "long";
  }
  return "invalid";
}

} // namespace

void DiagnosticRuntime::begin() {
  diagnostics_.begin();
  matrix_.begin();
  button_.begin();

  const std::uint64_t now_us = clock_.now_us();
  controller_.initialize(static_cast<std::uint32_t>(now_us / 1000U));

  board::print_foundation_probe();
  const bool imu_ok = imu_.begin();
  const auto status = imu_.status();

  char line[192];
  std::snprintf(line, sizeof(line),
                "espsand.io start firmware=es002-dev matrix_gpio=%d brightness_ceiling=%u "
                "imu_init=%u imu_addr=0x%02X who=0x%02X revision=0x%02X",
                board::kMatrixData.gpio, matrix_.brightness_ceiling(), imu_ok ? 1U : 0U,
                status.address, status.who_am_i, status.revision);
  diagnostics_.write_line(line);

  next_imu_us_ = now_us;
  next_render_us_ = now_us;
  next_telemetry_us_ = now_us + kTelemetryPeriodUs;
  telemetry_window_start_us_ = now_us;
}

void DiagnosticRuntime::tick() {
  const std::uint64_t loop_start_us = clock_.now_us();
  const std::uint64_t now_us = loop_start_us;

  const io::ButtonEvent event = button_.poll(now_us);
  if (event != io::ButtonEvent::kNone) {
    controller_.handle_button(event, static_cast<std::uint32_t>(now_us / 1000U));
    emit_button_event(event);
    next_render_us_ = now_us;
  }

  if (now_us >= next_imu_us_) {
    next_imu_us_ = now_us + kImuPeriodUs;
    io::ImuSample sample;
    if (imu_.poll(sample)) {
      latest_imu_ = sample;
      ++imu_samples_window_;
    }
  }

  if (now_us >= next_render_us_) {
    next_render_us_ = now_us + kRenderPeriodUs;
    render(now_us);
  }

  if (now_us >= next_telemetry_us_) {
    emit_telemetry(now_us);
    next_telemetry_us_ = now_us + kTelemetryPeriodUs;
  }

  const std::uint64_t loop_elapsed_us = clock_.now_us() - loop_start_us;
  max_loop_us_ = std::max(max_loop_us_, loop_elapsed_us);
}

void DiagnosticRuntime::render(std::uint64_t now_us) {
  io::Frame8x8 frame{};
  const std::uint32_t now_ms = static_cast<std::uint32_t>(now_us / 1000U);
  const std::uint32_t elapsed_ms = controller_.elapsed_ms(now_ms);
  std::uint8_t requested_brightness = kNormalBrightness;

  switch (controller_.mode()) {
  case runtime::DiagnosticMode::kPixelSweep:
    render_pixel_sweep(frame, elapsed_ms);
    break;
  case runtime::DiagnosticMode::kPrimaryColours:
    render_primary_colours(frame, elapsed_ms);
    requested_brightness = kPrimaryBrightness;
    break;
  case runtime::DiagnosticMode::kGravity:
    render_gravity(frame);
    break;
  case runtime::DiagnosticMode::kCount:
    break;
  }

  overlay_fault(frame, elapsed_ms);
  matrix_.present(frame, requested_brightness);
}

void DiagnosticRuntime::render_pixel_sweep(io::Frame8x8& frame,
                                           std::uint32_t elapsed_ms) const {
  const std::size_t index = (elapsed_ms / 100U) % io::kMatrixPixels;
  frame[index] = {255, 96, 0};
}

void DiagnosticRuntime::render_primary_colours(io::Frame8x8& frame,
                                               std::uint32_t elapsed_ms) const {
  const std::uint32_t phase = (elapsed_ms / 700U) % 3U;
  io::Rgb colour{};
  if (phase == 0U) {
    colour = {255, 0, 0};
  } else if (phase == 1U) {
    colour = {0, 255, 0};
  } else {
    colour = {0, 0, 255};
  }
  frame.fill(colour);
}

void DiagnosticRuntime::render_gravity(io::Frame8x8& frame) const {
  if (!latest_imu_.valid) {
    return;
  }

  const io::Vec2 gravity =
      input::project_to_matrix(latest_imu_.accel_g, board::kProvisionalMatrixTransform);
  const float gx = std::clamp(gravity.x, -1.0F, 1.0F);
  const float gy = std::clamp(gravity.y, -1.0F, 1.0F);
  const int x = static_cast<int>(std::lround((gx + 1.0F) * 3.5F));
  const int y = static_cast<int>(std::lround((gy + 1.0F) * 3.5F));
  frame[pixel_index(x, y)] = {0, 180, 255};
  frame[pixel_index(3, 3)] = {0, 10, 10};
}

void DiagnosticRuntime::overlay_fault(io::Frame8x8& frame,
                                      std::uint32_t elapsed_ms) const {
  const auto status = imu_.status();
  if (status.healthy || ((elapsed_ms / 250U) % 2U) == 0U) {
    return;
  }
  frame[pixel_index(0, 0)] = {255, 0, 0};
  frame[pixel_index(7, 0)] = {255, 0, 0};
  frame[pixel_index(0, 7)] = {255, 0, 0};
  frame[pixel_index(7, 7)] = {255, 0, 0};
}

void DiagnosticRuntime::emit_button_event(io::ButtonEvent event) {
  char line[128];
  std::snprintf(line, sizeof(line), "input.button event=%s mode=%s", event_name(event),
                runtime::diagnostic_mode_name(controller_.mode()));
  diagnostics_.write_line(line);
}

void DiagnosticRuntime::emit_telemetry(std::uint64_t now_us) {
  const auto status = imu_.status();
  const std::uint64_t window_us = now_us - telemetry_window_start_us_;
  const float imu_rate_hz = window_us == 0
                                ? 0.0F
                                : static_cast<float>(imu_samples_window_) * 1000000.0F /
                                      static_cast<float>(window_us);

  char line[320];
  std::snprintf(
      line, sizeof(line),
      "runtime mode=%s imu_ok=%u imu_addr=0x%02X imu_rate_hz=%.1f failures=%lu "
      "acc_g=(%+.3f,%+.3f,%+.3f) gyro_dps=(%+.2f,%+.2f,%+.2f) max_loop_us=%llu",
      runtime::diagnostic_mode_name(controller_.mode()), status.healthy ? 1U : 0U,
      status.address, static_cast<double>(imu_rate_hz),
      static_cast<unsigned long>(status.failure_count), static_cast<double>(latest_imu_.accel_g.x),
      static_cast<double>(latest_imu_.accel_g.y), static_cast<double>(latest_imu_.accel_g.z),
      static_cast<double>(latest_imu_.gyro_dps.x), static_cast<double>(latest_imu_.gyro_dps.y),
      static_cast<double>(latest_imu_.gyro_dps.z),
      static_cast<unsigned long long>(max_loop_us_));
  diagnostics_.write_line(line);

  telemetry_window_start_us_ = now_us;
  imu_samples_window_ = 0;
  max_loop_us_ = 0;
}

} // namespace espsand::app
