#include "scene_runtime.hpp"

#include <algorithm>
#include <cstdio>

#include <espsand/render/scene_effects.hpp>
#include <espsand/sim/materials.hpp>

#include "board/board_profile.hpp"

namespace espsand::app {
namespace {

constexpr std::uint64_t kImuPeriodUs = 5000;
constexpr std::uint64_t kSimulationPeriodUs = 16667;
constexpr std::uint64_t kRenderPeriodUs = 16667;
constexpr std::uint64_t kTelemetryPeriodUs = 1000000;
constexpr std::uint64_t kDefaultSceneSeed = 0xE5007007ULL;

struct SceneRenderSettings {
  std::uint16_t exposure_q8 = 320;
  std::uint8_t brightness = 28;
  std::uint8_t persistence_q8 = 56;
};

SceneRenderSettings render_settings(sim::SceneId scene) noexcept {
  switch (scene) {
  case sim::SceneId::kSodiumWater:
    return {340, 26, 48};
  case sim::SceneId::kOilFire:
    return {300, 28, 64};
  case sim::SceneId::kMossGarden:
    return {300, 26, 96};
  case sim::SceneId::kLavaWater:
  case sim::SceneId::kDeterminismFixture:
  case sim::SceneId::kDynamicsFixture:
    return {320, 28, 56};
  }
  return {320, 28, 56};
}

std::uint32_t material_mass(const sim::MaterialTotals& totals, sim::MaterialId material) noexcept {
  return totals.mass[sim::material_index(material)];
}

float rate_hz(std::uint32_t count, std::uint64_t window_us) noexcept {
  if (window_us == 0U) {
    return 0.0F;
  }
  return static_cast<float>(count) * 1000000.0F / static_cast<float>(window_us);
}

bool is_reaction_highlight_material(sim::MaterialId material) noexcept {
  return material == sim::MaterialId::kSteam || material == sim::MaterialId::kFire;
}

void highlight_hot_reaction(io::Frame8x8& frame, const sim::World& world,
                            std::uint16_t reactions_applied) noexcept {
  if (reactions_applied == 0U) {
    return;
  }

  const sim::Cell* hottest = nullptr;
  std::size_t hottest_index = 0;
  for (std::size_t index = 0; index < sim::kWorldCapacity; ++index) {
    const sim::Cell* cell = world.try_cell_index(index);
    if (cell == nullptr || !is_reaction_highlight_material(cell->material) || cell->mass == 0U) {
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
  configure_scene(sim::SceneId::kLavaWater, kDefaultSceneSeed);

  const std::uint64_t now_us = clock_.now_us();
  next_imu_us_ = now_us;
  next_sim_us_ = now_us;
  next_render_us_ = now_us;
  next_telemetry_us_ = now_us + kTelemetryPeriodUs;
  telemetry_window_start_us_ = now_us;

  const auto imu_status = imu_.status();
  const auto touch_status = touch_.status();
  const SceneRenderSettings settings = render_settings(model_.scene());
  char line[288];
  std::snprintf(line, sizeof(line),
                "espsand.scene start scene=%s seed=%llu scene_count=%u sim_target_hz=60 "
                "brightness=%u imu_init=%u imu_addr=0x%02X touch_hw=%u slider=%u combo=%u",
                sim::scene_name(model_.scene()), static_cast<unsigned long long>(model_.seed()),
                static_cast<unsigned>(sim::kProductSceneOrder.size()), settings.brightness,
                imu_ok ? 1U : 0U, imu_status.address, touch_ok ? 1U : 0U,
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

void SceneRuntime::configure_scene(sim::SceneId scene, std::uint64_t seed) {
  sim::ModelConfig config{};
  config.seed = seed;
  config.scene = scene;
  config.event_budget = 12;
  config.reaction_budget = 12;
  model_.init(config);
  latest_render_stats_ = {};
  previous_frame_ = {};
  has_previous_frame_ = false;
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
  const SceneRenderSettings settings = render_settings(model_.scene());
  render::RenderConfig config{};
  config.mode = render::RenderMode::kBeauty;
  config.exposure_q8 = settings.exposure_q8;

  render::RenderResult result = renderer_.render(model_.world(), config);
  if (has_previous_frame_) {
    render::blend_with_previous(result.frame, previous_frame_, settings.persistence_q8);
  }
  previous_frame_ = result.frame;
  has_previous_frame_ = true;

  highlight_hot_reaction(result.frame, model_.world(), model_.dynamics_stats().reactions_applied);
  if (model_.scene() == sim::SceneId::kMossGarden) {
    render::apply_mite_overlay(result.frame, model_.moss_garden_state());
  }

  latest_render_stats_ = result.stats;
  matrix_.present(result.frame, settings.brightness);
  ++rendered_frames_window_;
}

void SceneRuntime::handle_button(io::ButtonEvent event) {
  char line[192];
  if (event == io::ButtonEvent::kShortPress) {
    model_.reset();
    previous_frame_ = {};
    has_previous_frame_ = false;
    const char* name = sim::scene_name(model_.scene());
    const auto seed = static_cast<unsigned long long>(model_.seed());
    std::snprintf(line, sizeof(line), "input.button event=short action=reset scene=%s seed=%llu",
                  name, seed);
    diagnostics_.write_line(line);
  } else if (event == io::ButtonEvent::kLongPress) {
    const sim::SceneId previous = model_.scene();
    const sim::SceneId next = sim::next_product_scene(previous);
    const std::uint64_t next_seed = model_.seed() + 1U;
    configure_scene(next, next_seed);
    std::snprintf(line, sizeof(line),
                  "input.button event=long action=next_scene from=%s to=%s seed=%llu",
                  sim::scene_name(previous), sim::scene_name(next),
                  static_cast<unsigned long long>(next_seed));
    diagnostics_.write_line(line);
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
  const render::OutputDecision output = matrix_.last_output_decision();

  char line[512];
  std::snprintf(
      line, sizeof(line),
      "runtime scene=%s seed=%llu tick=%llu hash=%016llX imu_hz=%.1f sim_hz=%.1f "
      "render_hz=%.1f max_sim_us=%llu max_loop_us=%llu g=(%+.2f,%+.2f) shake=%.2f tap=%.2f",
      sim::scene_name(model_.scene()), static_cast<unsigned long long>(model_.seed()),
      static_cast<unsigned long long>(model_.tick()),
      static_cast<unsigned long long>(model_.state_hash()), static_cast<double>(imu_rate_hz),
      static_cast<double>(sim_rate_hz), static_cast<double>(render_rate_hz),
      static_cast<unsigned long long>(max_sim_tick_us_),
      static_cast<unsigned long long>(max_loop_us_), static_cast<double>(latest_input_.gravity.x),
      static_cast<double>(latest_input_.gravity.y), static_cast<double>(latest_input_.shake_energy),
      static_cast<double>(latest_input_.tap_impulse));
  diagnostics_.write_line(line);
  emit_scene_telemetry(totals, work, dynamics, output);

  telemetry_window_start_us_ = now_us;
  imu_samples_window_ = 0;
  sim_ticks_window_ = 0;
  rendered_frames_window_ = 0;
  max_loop_us_ = 0;
  max_sim_tick_us_ = 0;
}

void SceneRuntime::emit_scene_telemetry(const sim::MaterialTotals& totals,
                                        const sim::TickWorkStats& work,
                                        const sim::DynamicsStats& dynamics,
                                        const render::OutputDecision& output) {
  char line[512];
  // clang-format off
  // Keep each compact telemetry schema aligned with its argument order for field-level review.
  switch (model_.scene()) {
  case sim::SceneId::kLavaWater: {
    const sim::LavaWaterSceneStats scene = model_.lava_water_stats();
    std::snprintf(
        line, sizeof(line),
        "scene.lava_water water=%lu lava=%lu crust=%lu steam=%lu react=%u move=%u "
        "fracture=%u inject_lava=%u inject_water=%u touch_lava=%u burst=%u "
        "event=%u/%u/%u reaction=%u/%u/%u render_minor=%u led=%u/%u load=%lu limited=%u",
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
    break;
  }
  case sim::SceneId::kSodiumWater: {
    const sim::SodiumWaterSceneStats scene = model_.sodium_water_stats();
    std::snprintf(
        line, sizeof(line),
        "scene.sodium_water water=%lu sodium=%lu fire=%lu steam=%lu react=%u impulse=%u "
        "move=%u inject_sodium=%u inject_water=%u touch_sodium=%u burst=%u "
        "event=%u/%u/%u reaction=%u/%u/%u led=%u/%u load=%lu limited=%u",
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kWater)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kSodiumLike)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kFire)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kSteam)),
        dynamics.reactions_applied, dynamics.reaction_impulses, dynamics.transport_moves,
        scene.autonomous_sodium_injections, scene.autonomous_water_injections,
        scene.touch_sodium_injections, scene.burst_pairs, work.events.used, work.events.limit,
        work.events.dropped, work.reactions.used, work.reactions.limit, work.reactions.dropped,
        output.requested_brightness, output.applied_brightness,
        static_cast<unsigned long>(output.estimated_frame_load),
        output.ceiling_limited || output.load_limited ? 1U : 0U);
    break;
  }
  case sim::SceneId::kOilFire: {
    const sim::OilFireSceneStats scene = model_.oil_fire_stats();
    std::snprintf(
        line, sizeof(line),
        "scene.oil_fire water=%lu oil=%lu fire=%lu smoke=%lu react=%u expired=%u move=%u "
        "inject_oil=%u auto_ignite=%u touch_oil=%u combo_ignite=%u "
        "event=%u/%u/%u reaction=%u/%u/%u led=%u/%u load=%lu limited=%u",
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kWater)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kOil)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kFire)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kSmoke)),
        dynamics.reactions_applied, dynamics.fire_cells_expired, dynamics.transport_moves,
        scene.autonomous_oil_injections, scene.autonomous_ignitions, scene.touch_oil_injections,
        scene.combo_ignitions, work.events.used, work.events.limit, work.events.dropped,
        work.reactions.used, work.reactions.limit, work.reactions.dropped,
        output.requested_brightness, output.applied_brightness,
        static_cast<unsigned long>(output.estimated_frame_load),
        output.ceiling_limited || output.load_limited ? 1U : 0U);
    break;
  }
  case sim::SceneId::kMossGarden: {
    const sim::MossGardenSceneStats scene = model_.moss_garden_stats();
    const sim::MossGardenStateSnapshot state = model_.moss_garden_state();
    std::snprintf(
        line, sizeof(line),
        "scene.moss_garden water=%lu moss=%lu mites=%u growth_energy=%u grow=%u reinforce=%u "
        "mite_move=%u feed=%u starved=%u rain=%u seed=%u scatter=%u "
        "event=%u/%u/%u led=%u/%u load=%lu limited=%u",
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kWater)),
        static_cast<unsigned long>(material_mass(totals, sim::MaterialId::kMoss)),
        state.mite_count, state.growth_energy, scene.growth_cells, scene.reinforced_cells,
        scene.mite_moves, scene.feeds, scene.starved, scene.rain_pulses, scene.seed_pulses,
        scene.scatter_events, work.events.used, work.events.limit, work.events.dropped,
        output.requested_brightness, output.applied_brightness,
        static_cast<unsigned long>(output.estimated_frame_load),
        output.ceiling_limited || output.load_limited ? 1U : 0U);
    break;
  }
  case sim::SceneId::kDeterminismFixture:
  case sim::SceneId::kDynamicsFixture:
    std::snprintf(line, sizeof(line), "scene.%s unexpected_product_runtime=1",
                  sim::scene_name(model_.scene()));
    break;
  }
  // clang-format on
  diagnostics_.write_line(line);
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
