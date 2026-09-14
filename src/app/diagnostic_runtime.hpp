#pragma once

#include <cstdint>

#include <espsand/io/touch_types.hpp>
#include <espsand/io/types.hpp>
#include <espsand/runtime/diagnostic_controller.hpp>

#include "board/boot_button.hpp"
#include "board/matrix_output.hpp"
#include "board/monotonic_clock.hpp"
#include "board/qmi8658_imu.hpp"
#include "board/serial_diagnostics.hpp"
#include "board/touch_zones.hpp"

namespace espsand::app {

class DiagnosticRuntime {
public:
  void begin();
  void tick();

private:
  void render(std::uint64_t now_us);
  void render_pixel_sweep(io::Frame8x8& frame, std::uint32_t elapsed_ms) const;
  void render_primary_colours(io::Frame8x8& frame, std::uint32_t elapsed_ms) const;
  void render_gravity(io::Frame8x8& frame) const;
  void render_touch_characterization(io::Frame8x8& frame) const;
  void overlay_fault(io::Frame8x8& frame, std::uint32_t elapsed_ms) const;
  void emit_button_event(io::ButtonEvent event);
  void emit_telemetry(std::uint64_t now_us);
  void emit_touch_telemetry(std::uint64_t now_us);

  board::MonotonicClock clock_{};
  board::MatrixOutput matrix_{};
  board::Qmi8658Imu imu_{};
  board::BootButton button_{};
  board::TouchZones touch_{};
  board::SerialDiagnostics diagnostics_{};
  runtime::DiagnosticController controller_{};
  io::ImuSample latest_imu_{};
  io::TouchFrame latest_touch_{};

  std::uint64_t next_imu_us_ = 0;
  std::uint64_t next_render_us_ = 0;
  std::uint64_t next_telemetry_us_ = 0;
  std::uint64_t next_touch_telemetry_us_ = 0;
  std::uint64_t telemetry_window_start_us_ = 0;
  std::uint64_t max_loop_us_ = 0;
  std::uint32_t imu_samples_window_ = 0;
};

} // namespace espsand::app
