#pragma once

#include <cstdint>

#include <espsand/input/motion_interpreter.hpp>
#include <espsand/io/touch_types.hpp>
#include <espsand/io/types.hpp>
#include <espsand/render/world_renderer.hpp>
#include <espsand/sim/input_frame.hpp>
#include <espsand/sim/model.hpp>

#include "board/boot_button.hpp"
#include "board/matrix_output.hpp"
#include "board/monotonic_clock.hpp"
#include "board/qmi8658_imu.hpp"
#include "board/serial_diagnostics.hpp"
#include "board/touch_zones.hpp"

namespace espsand::app {

class SceneRuntime {
public:
  void begin();
  void tick();

private:
  sim::InputFrame compose_input() const noexcept;
  void configure_scene(sim::SceneId scene, std::uint64_t seed);
  void run_simulation_tick(std::uint64_t now_us);
  void render_scene();
  void handle_button(io::ButtonEvent event);
  void emit_telemetry(std::uint64_t now_us);
  void emit_scene_telemetry(const sim::MaterialTotals& totals, const sim::TickWorkStats& work,
                            const sim::DynamicsStats& dynamics,
                            const render::OutputDecision& output);
  void clear_one_shot_inputs() noexcept;

  board::MonotonicClock clock_{};
  board::MatrixOutput matrix_{};
  board::Qmi8658Imu imu_{};
  board::BootButton button_{};
  board::TouchZones touch_{};
  board::SerialDiagnostics diagnostics_{};

  input::MotionInterpreter motion_{};
  render::WorldRenderer renderer_{};
  sim::Model model_{};
  io::ImuSample latest_imu_{};
  io::TouchFrame latest_touch_{};
  sim::InputFrame latest_input_{};
  render::RenderStats latest_render_stats_{};

  std::uint64_t next_imu_us_ = 0;
  std::uint64_t next_sim_us_ = 0;
  std::uint64_t next_render_us_ = 0;
  std::uint64_t next_telemetry_us_ = 0;
  std::uint64_t telemetry_window_start_us_ = 0;
  std::uint64_t max_loop_us_ = 0;
  std::uint64_t max_sim_tick_us_ = 0;
  std::uint32_t imu_samples_window_ = 0;
  std::uint32_t sim_ticks_window_ = 0;
  std::uint32_t rendered_frames_window_ = 0;
};

} // namespace espsand::app
