#include <unity.h>

#include <array>

#include <espsand/input/axis_transform.hpp>
#include <espsand/input/button_gesture.hpp>
#include <espsand/input/touch_normalizer.hpp>
#include <espsand/io/interfaces.hpp>
#include <espsand/io/null_touch_zones.hpp>
#include <espsand/runtime/diagnostic_controller.hpp>

namespace {

void test_short_press_after_debounce() {
  espsand::input::ButtonGesture button;
  button.reset(false, 0);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(true, 10)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(true, 40)));
  TEST_ASSERT_TRUE(button.stable_pressed());

  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(false, 200)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kShortPress),
                        static_cast<int>(button.update(false, 230)));
}

void test_long_press_emits_once_and_not_again_on_release() {
  espsand::input::ButtonGesture button;
  button.reset(false, 0);
  button.update(true, 10);
  button.update(true, 40);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kLongPress),
                        static_cast<int>(button.update(true, 840)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(true, 900)));
  button.update(false, 920);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(false, 950)));
}

void test_ambiguous_duration_does_not_fire_short_or_long() {
  espsand::input::ButtonGesture button;
  button.reset(false, 0);
  button.update(true, 10);
  button.update(true, 40);
  button.update(false, 740);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(false, 770)));
}

void test_bounce_does_not_become_a_press() {
  espsand::input::ButtonGesture button;
  button.reset(false, 0);
  button.update(true, 5);
  button.update(false, 15);
  button.update(true, 20);
  button.update(false, 25);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::io::ButtonEvent::kNone),
                        static_cast<int>(button.update(false, 100)));
  TEST_ASSERT_FALSE(button.stable_pressed());
}

void test_axis_projection_is_explicit_and_rotatable() {
  const espsand::io::Vec3 value{1.0F, 2.0F, 3.0F};
  const espsand::input::PlaneTransform transform{
      {espsand::input::Axis::kY, -1},
      {espsand::input::Axis::kX, 1},
  };

  const auto projected = espsand::input::project_to_matrix(value, transform);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, -2.0F, projected.x);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 1.0F, projected.y);
}

void test_diagnostic_controller_reset_and_next_mode() {
  espsand::runtime::DiagnosticController controller;
  controller.initialize(100);
  TEST_ASSERT_EQUAL_UINT32(250, controller.elapsed_ms(350));

  controller.handle_button(espsand::io::ButtonEvent::kShortPress, 400);
  TEST_ASSERT_EQUAL_UINT32(0, controller.elapsed_ms(400));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::runtime::DiagnosticMode::kPixelSweep),
                        static_cast<int>(controller.mode()));

  controller.handle_button(espsand::io::ButtonEvent::kLongPress, 500);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::runtime::DiagnosticMode::kPrimaryColours),
                        static_cast<int>(controller.mode()));
  controller.handle_button(espsand::io::ButtonEvent::kLongPress, 600);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::runtime::DiagnosticMode::kGravity),
                        static_cast<int>(controller.mode()));
  controller.handle_button(espsand::io::ButtonEvent::kLongPress, 700);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::runtime::DiagnosticMode::kTouchCharacterization),
                        static_cast<int>(controller.mode()));
  TEST_ASSERT_EQUAL_UINT32(0, controller.elapsed_ms(700));
  controller.handle_button(espsand::io::ButtonEvent::kLongPress, 800);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(espsand::runtime::DiagnosticMode::kPixelSweep),
                        static_cast<int>(controller.mode()));
}

std::array<std::uint32_t, espsand::io::kMaxTouchChannels> stable_touch_raw() {
  std::array<std::uint32_t, espsand::io::kMaxTouchChannels> raw{};
  raw[0] = 1000;
  raw[1] = 1200;
  raw[2] = 1400;
  return raw;
}

espsand::io::TouchDiagnostics warm_touch_normalizer(espsand::input::TouchNormalizer& normalizer) {
  auto raw = stable_touch_raw();
  espsand::io::TouchDiagnostics diagnostics{};
  for (int sample = 0; sample < 40; ++sample) {
    raw[0] = static_cast<std::uint32_t>(1000 + ((sample % 3) - 1));
    raw[1] = static_cast<std::uint32_t>(1200 + (((sample + 1) % 3) - 1));
    raw[2] = static_cast<std::uint32_t>(1400 + (((sample + 2) % 3) - 1));
    diagnostics = normalizer.update(raw, 3);
  }
  return diagnostics;
}

void test_touch_normalizer_tracks_idle_without_false_activation() {
  espsand::input::TouchNormalizer normalizer;
  normalizer.reset(3);
  const auto diagnostics = warm_touch_normalizer(normalizer);

  TEST_ASSERT_TRUE(diagnostics.ready);
  TEST_ASSERT_FALSE(diagnostics.channels[0].active);
  TEST_ASSERT_FALSE(diagnostics.channels[1].active);
  TEST_ASSERT_FALSE(diagnostics.channels[2].active);
  TEST_ASSERT_FLOAT_WITHIN(2.0F, 0.0F, diagnostics.common_mode_z);
}

void test_touch_normalizer_detects_local_positive_s3_touch() {
  espsand::input::TouchNormalizer normalizer;
  normalizer.reset(3);
  warm_touch_normalizer(normalizer);

  auto raw = stable_touch_raw();
  raw[0] += 100;
  const auto diagnostics = normalizer.update(raw, 3);

  TEST_ASSERT_TRUE(diagnostics.channels[0].active);
  TEST_ASSERT_TRUE(diagnostics.channels[0].z > 5.0F);
  TEST_ASSERT_FALSE(diagnostics.channels[1].active);
  TEST_ASSERT_FALSE(diagnostics.channels[2].active);
}

void test_touch_normalizer_rejects_equal_common_mode_shift() {
  espsand::input::TouchNormalizer normalizer;
  normalizer.reset(3);
  warm_touch_normalizer(normalizer);

  auto raw = stable_touch_raw();
  raw[0] += 100;
  raw[1] += 100;
  raw[2] += 100;
  const auto diagnostics = normalizer.update(raw, 3);

  TEST_ASSERT_TRUE(diagnostics.common_mode_z > 10.0F);
  TEST_ASSERT_FLOAT_WITHIN(1.0F, 0.0F, diagnostics.channels[0].z);
  TEST_ASSERT_FLOAT_WITHIN(1.0F, 0.0F, diagnostics.channels[1].z);
  TEST_ASSERT_FLOAT_WITHIN(1.0F, 0.0F, diagnostics.channels[2].z);
  TEST_ASSERT_FALSE(diagnostics.channels[0].active);
  TEST_ASSERT_FALSE(diagnostics.channels[1].active);
  TEST_ASSERT_FALSE(diagnostics.channels[2].active);
}

void test_touch_gate_hysteresis_and_cooldown_bound_events() {
  espsand::input::TouchGateConfig config{};
  config.cooldown_samples = 3;
  espsand::input::TouchZoneGate gate(config);

  auto state = gate.update(6.0F);
  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_TRUE(state.triggered);

  state = gate.update(6.0F);
  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_FALSE(state.triggered);

  state = gate.update(2.0F);
  TEST_ASSERT_FALSE(state.active);
  state = gate.update(6.0F);
  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_FALSE(state.triggered);

  state = gate.update(2.0F);
  TEST_ASSERT_FALSE(state.active);
  state = gate.update(6.0F);
  TEST_ASSERT_TRUE(state.triggered);
}

void test_null_touch_zones_are_cleanly_unavailable() {
  espsand::io::NullTouchZones touch;
  espsand::io::TouchFrame frame{};
  frame.available = true;
  frame.cap_a = 1.0F;

  TEST_ASSERT_FALSE(touch.poll(12345, frame));
  TEST_ASSERT_FALSE(frame.available);
  TEST_ASSERT_EQUAL_UINT64(12345, frame.timestamp_us);
  TEST_ASSERT_FLOAT_WITHIN(0.0001F, 0.0F, frame.cap_a);
  TEST_ASSERT_FALSE(touch.status().hardware_available);
  TEST_ASSERT_EQUAL_UINT8(0, touch.diagnostics().channel_count);
}

class FakeClock final : public espsand::io::IClock {
public:
  std::uint64_t now_us() const override {
    return now_us_;
  }
  std::uint64_t now_us_ = 1234;
};

class FakeImu final : public espsand::io::IImu {
public:
  bool poll(espsand::io::ImuSample& sample) override {
    sample = sample_;
    return sample.valid;
  }

  espsand::io::ImuStatus status() const override {
    return status_;
  }

  espsand::io::ImuSample sample_{};
  espsand::io::ImuStatus status_{};
};

class FakeMatrix final : public espsand::io::IMatrixOutput {
public:
  void present(const espsand::io::Frame8x8& frame, std::uint8_t requested_brightness) override {
    frame_ = frame;
    brightness_ = requested_brightness;
  }

  std::uint8_t brightness_ceiling() const override {
    return 32;
  }

  espsand::io::Frame8x8 frame_{};
  std::uint8_t brightness_ = 0;
};

void test_hardware_interfaces_accept_host_fakes() {
  FakeClock clock;
  FakeImu imu;
  FakeMatrix matrix;

  imu.sample_.valid = true;
  espsand::io::ImuSample sample;
  TEST_ASSERT_TRUE(imu.poll(sample));
  TEST_ASSERT_EQUAL_UINT64(1234, clock.now_us());

  espsand::io::Frame8x8 frame{};
  frame[0] = {1, 2, 3};
  matrix.present(frame, 17);
  TEST_ASSERT_EQUAL_UINT8(17, matrix.brightness_);
  TEST_ASSERT_EQUAL_UINT8(1, matrix.frame_[0].r);
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_short_press_after_debounce);
  RUN_TEST(test_long_press_emits_once_and_not_again_on_release);
  RUN_TEST(test_ambiguous_duration_does_not_fire_short_or_long);
  RUN_TEST(test_bounce_does_not_become_a_press);
  RUN_TEST(test_axis_projection_is_explicit_and_rotatable);
  RUN_TEST(test_diagnostic_controller_reset_and_next_mode);
  RUN_TEST(test_touch_normalizer_tracks_idle_without_false_activation);
  RUN_TEST(test_touch_normalizer_detects_local_positive_s3_touch);
  RUN_TEST(test_touch_normalizer_rejects_equal_common_mode_shift);
  RUN_TEST(test_touch_gate_hysteresis_and_cooldown_bound_events);
  RUN_TEST(test_null_touch_zones_are_cleanly_unavailable);
  RUN_TEST(test_hardware_interfaces_accept_host_fakes);
  return UNITY_END();
}
