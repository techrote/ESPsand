#include "scene_runtime.hpp"

#include <algorithm>
#include <cstdio>

#include <espsand/sim/materials.hpp>

#include "board/board_profile.hpp"

namespace espsand::app {
namespace {

constexpr std::uint64_t kImuPeriodUs = 5000;
constexpr std::uint64_t kSimulationPeriodUs = 16667;
constexpr std::uint64_t kRenderPeriodUs = 16667;
constexpr std::uint64_t kTelemetryPeriodUs = 1000000;
constexpr std::uint64_t kDefaultSceneSeed = 0xE5007007ULL;
constexpr std::uint8_t kSceneBrightness = 28;
constexpr std::uint16_t kSceneExposureQ8 = 320;

std::uint32_t material_mass(const sim::MaterialTotals& totals, sim::MaterialId material) noexcept {
  return totals.mass[sim::material_index(material)];
}

float rate_hz(std::uint32_t count, std::uint64_t window_us) noexcept {
  if (window_us == 0U) {
    return 0.0F;
  }
  return static_cast<float>(count) * 1000000.0F / static_cast<float>(window_us);
}

void highlight_hot_steam(io::Frame8x8& frame, const sim::World& world,
                         std::uint16_t reactions_applied) noexcept {
  if (reactions_applied == 0U) {
    return;
  }

  const sim::Cell* hottest = nullptr;
  std::size_t hottest_index = 0;
  for (std::size_t index = 0; index < sim::kWorldCapacity; ++index) {
    const sim::Cell* cell = world.try_cell_index(index);
    if (cell == nullptr || cell->material != sim::MaterialId::kSteam || cell->mass == 0U) {
      continue;
    }
    if (hottest == nullptr || cell->temperature > hottest->temperature) {
      hottest = cell;
      hottest_index = index;
    }
  }

  if (hottest == nullptr) {
    return;
  }

  const std::size_t logical_x = hottest_index % sim::kWorldWidth;
  const std::size_t logical_y = hottest_index / sim::kWorldWidth;
  const std::size_t output_x = logical_x / 2U;
  const std::size_t output_y = logical_y / 2U;
  io::Rgb& pixel = frame[output_y * io::kMatrixWidth + output_x];
  pixel.r = std::max<std::uint8_t>(pixel.r, 240U);
  pixel.g = std::max<std::uint8_t>(pixel.g, 220U);
  pixel.b = std::max<std::uint8_t>(pixel.b, 180U);
}

} // namespace

void SceneRuntime::begin() {
  diagnostics_.begin();
  matrix_.begin();
  button_.begin();
  const bool touch_ok = touch_.begin();
  const bool imu_ok = imu_.begin();
  motion_.reset();

  sim::ModelConfig config{};
  config.seed = kDefaultSceneSeed;
  config.scene = sim::SceneId::kLavaWater;
  config.event_budget = 12;
  config.reaction_budget = 12;
  model_.init(config);

  const std::uint64_t now_us = clock_.now_us();
  next_imu_us_ = now_us;
  next_sim_us_ = now_us;
  next_render_us_ = now_us;
  next_telemetry_us_ = now_us + kTelemetryPeriodUs;
  telemetry_window_start_us_ = now_us;

  const auto imu_status = imu_.status();
  const auto touch_status = touch_.status();
  char line[256];
  std::snprintf(line, sizeof(line),
                "espsand.scene start scene=lava_water seed=%llu sim_target_hz=60 "
                "brightness=%u imu_init=%u imu_addr=0x%02X touch_hw=%u slider=%u combo=%u",
                static_cast<unsigned long long>(model_.seed()), kSceneBrightness, imu_ok ? 1U : 0U,
                imu_status.address, touch_ok ? 1U : 0U,
                touch_status.hardware_available && board::kTouchSliderConfigured ? 1U : 0U,
                touch_status.hardware_available && board::kTouchComboConfigured ? 1U : 0U);
  diagnostics_.write_line(line);
}

void SceneRuntime::tick() {
  const std::uint64_t loop_start_us = clock_.now_us();
  const std::uint64_t now_us = loop_start_us;

  const io::ButtonEvent event = button_.poll(now_us);
  if (event != io::ButtonEvent::kNone) {
    handle_button(event);
  }

  if (now_us >= next_imu_us_) {
    next_imu_us_ = now_us + kImuPeriodUs;
    io::ImuSample sample;
    if (imu_.poll(sample)) {
      latest_imu_ = sample;
      motion_.update(sample, board::kProvisionalMatrixTransform);
      ++imu_samples_window_;
    }
  }

  io::TouchFrame touch_frame;
  if (touch_.poll(now_us, touch_frame)) {
    latest_touch_ = touch_frame;
  }

  if (now_us >= next_sim_us_) {
    next_sim_us_ = now_us + kSimulationPeriodUs;
    run_simulation_tick(now_us);
  }

  if (now_us >= next_render_us_) {
    next_render_us_ = now_us + kRenderPeriodUs;
    render_scene();
  }

  if (now_us >= next_telemetry_us_) {
    emit_telemetry(now_us);
    next_telemetry_us_ = now_us + kTelemetryPeriodUs;
  }

  const std::uint64_t loop_elapsed_us = clock_.now_us() - loop_start_us;
  max_loop_us_ = std::max(max_loop_us_, loop_elapsed_us);
}

sim::InputFrame SceneRuntime::compose_input() const noexcept {
  sim::InputFrame frame{};
  const input::MotionSnapshot motion = motion_.snapshot();

  if (motion.ready) {
    frame.gravity = motion.gravity;
    frame.gravity_magnitude = motion.gravity_magnitude;
    frame.gravity_confidence = motion.gravity_confidence;
    frame.shake_energy = motion.shake_energy;
    frame.motion_energy = motion.motion_energy;
    frame.tap_impulse = motion.tap_impulse;
    frame.spin_rate = motion.spin_rate;
  } else {
    frame.gravity = {0.0F, 1.0F};
    frame.gravity_magnitude = 1.0F;
    frame.gravity_confidence = 0.5F;
  }

  if (latest_touch_.available) {
    frame.cap_combo = latest_touch_.cap_combo;
    frame.cap_combo_event = latest_touch_.event_combo;
    frame.slider_active = latest_touch_.slider_active;
    frame.slider_position = latest_touch_.slider_position;
    frame.slider_strength = latest_touch_.slider_strength;
    frame.noise_impulse = latest_touch_.noise_impulse;
    frame.noise_event = latest_touch_.noise_event;
  }

  return frame;
}

void SceneRuntime::run_simulation_tick(std::uint64_t now_us) {
  latest_input_ = compose_input();
  const std::uint64_t start_us = clock_.now_us();
  model_.step(latest_input_);
  const std::uint64_t elapsed_us = clock_.now_us() - start_us;
  max_sim_tick_us_ = std::max(max_sim_tick_us_, elapsed_us);
  ++sim_ticks_window_;
  clear_one_shot_inputs();
  static_cast<void>(now_us);
}

void SceneRuntime::render_scene() {
  render::RenderConfig config{};
  config.mode = render::RenderMode::kBeauty;
  config.exposure_q8 = kSceneExposureQ8;

  render::RenderResult result = renderer_.render(model_.world(), config);
  highlight_hot_steam(result.frame, model_.world(), model_.dynamics_stats().reactions_applied);
  latest_render_stats_ = result.stats;
  matrix_.present(result.frame, kSceneBrightness);
  ++rendered_frames_window_;
}

void SceneRuntime::handle_button(io::ButtonEvent event) {
  if (event == io::ButtonEvent::kShortPress) {
    model_.reset();
    diagnostics_.write_line("input.button event=short action=reset scene=lava_water");
  } else if (event == io::ButtonEvent::kLongPress) {
    model_.reseed(model_.seed() + 1U);
    diagnostics_.write_line(
        "input.button event=long action=next_scene wrap=lava_water reseed=1 scene_count=1");
  }

  latest_touch_ = {};
  motion_.consume_transients();
  const std::uint64_t now_us = clock_.now_us();
  next_sim_us_ = now_us;
  next_render_us_ = now_us;
}

void SceneRuntime::emit_telemetry(std::uint64_t now_us) {
  const std::uint64_t window_us = now_us - telemetry_window_start_us_;
  const float imu_rate_hz = rate_hz(imu_samples_window_, window_us);
  const float sim_rate_hz = rate_hz(sim_ticks_window_, window_us);
  const float render_rate_hz = rate_hz(rendered_frames_window_, window_us);

  const sim::MaterialTotals totals = model_.world().totals();
  const sim::TickWorkStats work = model_.tick_work_stats();
  const sim::DynamicsStats dynamics = model_.dynamics_stats();
  const sim::LavaWaterSceneStats scene = model_.lava_water_stats();
  const render::OutputDecision output = matrix_.last_output_decision();

  char line[512];
  std::snprintf(
      line, sizeof(line),
      "runtime scene=lava_water seed=%llu tick=%llu hash=%016llX imu_hz=%.1f sim_hz=%.1f "
      "render_hz=%.1f max_sim_us=%llu max_loop_us=%llu g=(%+.2f,%+.2f) shake=%.2f tap=%.2f",
      static_cast<unsigned long long>(model_.seed()),
      static_cast<unsigned long long>(model_.tick()),
      static_cast<unsigned long long>(model_.state_hash()), static_cast<double>(imu_rate_hz),
      static_cast<double>(sim_rate_hz), static_cast<double>(render_rate_hz),
      static_cast<unsigned long long>(max_sim_tick_us_),
      static_cast<unsigned long long>(max_loop_us_), static_cast<double>(latest_input_.gravity.x),
      static_cast<double>(latest_input_.gravity.y), static_cast<double>(latest_input_.shake_energy),
      static_cast<double>(latest_input_.tap_impulse));
  diagnostics_.write_line(line);

  std::snprintf(
      line, sizeof(line),
      "scene.lava_water water=%lu lava=%lu crust=%lu steam=%lu react=%u move=%u fracture=%u "
      "inject_lava=%u inject_water=%u touch_lava=%u burst=%u event=%u/%u/%u reaction=%u/%u/%u "
      "render_minor=%u led=%u/%u load=%lu limited=%u",
      static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kWater)),
      static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kLava)),
      static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kCrust)),
      static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kSteam)),
      dynamics.reactions_applied, dynamics.transport_moves, scene.crust_fractures,
      scene.autonomous_lava_injections, scene.autonomous_water_injections,
      scene.touch_lava_injections, scene.burst_pairs, work.events.used, work.events.limit,
      work.events.dropped, work.reactions.used, work.reactions.limit, work.reactions.dropped,
      latest_render_stats_.minority_preserved_pixels, output.requested_brightness,
      output.applied_brightness, static_cast<unsigned long>(output.estimated_frame_load),
      output.ceiling_limited || output.load_limited ? 1U : 0U);
  diagnostics_.write_line(line);

  telemetry_window_start_us_ = now_us;
  imu_samples_window_ = 0;
  sim_ticks_window_ = 0;
  rendered_frames_window_ = 0;
  max_loop_us_ = 0;
  max_sim_tick_us_ = 0;
}

void SceneRuntime::clear_one_shot_inputs() noexcept {
  latest_touch_.event_a = false;
  latest_touch_.event_b = false;
  latest_touch_.event_combo = false;
  latest_touch_.noise_event = false;
  latest_touch_.noise_impulse = 0.0F;
  motion_.consume_transients();
}

} // namespace espsand::app
