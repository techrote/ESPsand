#include <unity.h>

#include <cstddef>

#include <espsand/input/touch_semantics.hpp>

namespace {

espsand::io::TouchDiagnostics ready_touch() {
  espsand::io::TouchDiagnostics diagnostics{};
  diagnostics.ready = true;
  diagnostics.channel_count = 7;
  return diagnostics;
}

void activate(espsand::io::TouchDiagnostics& diagnostics, std::size_t index, float z) {
  diagnostics.channels[index].active = true;
  diagnostics.channels[index].z = z;
}

void test_slider_requires_full_common_mode_and_two_local_channels() {
  espsand::input::TouchSemanticInterpreter interpreter;
  auto diagnostics = ready_touch();
  diagnostics.common_mode_z = 10.0F;
  activate(diagnostics, 1, 8.0F);

  auto state = interpreter.update(diagnostics);
  TEST_ASSERT_FALSE(state.slider_active);

  activate(diagnostics, 4, 8.0F);
  diagnostics.common_mode_z = 9.0F;
  state = interpreter.update(diagnostics);
  TEST_ASSERT_FALSE(state.slider_active);

  diagnostics.common_mode_z = 10.0F;
  state = interpreter.update(diagnostics);
  TEST_ASSERT_TRUE(state.slider_active);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 5.0F / 12.0F, state.slider_position);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, state.slider_strength);
}

void test_slider_centroid_moves_smoothly_across_edge() {
  espsand::input::TouchSemanticInterpreter interpreter;
  auto left = ready_touch();
  left.common_mode_z = 10.0F;
  activate(left, 0, 10.0F);
  activate(left, 1, 10.0F);

  const auto left_state = interpreter.update(left);
  TEST_ASSERT_TRUE(left_state.slider_active);
  TEST_ASSERT_TRUE(left_state.slider_position < 0.2F);

  auto right = ready_touch();
  right.common_mode_z = 10.0F;
  activate(right, 5, 10.0F);
  activate(right, 6, 10.0F);

  const auto right_state = interpreter.update(right);
  TEST_ASSERT_TRUE(right_state.slider_active);
  TEST_ASSERT_TRUE(right_state.slider_position > left_state.slider_position);
  TEST_ASSERT_TRUE(right_state.slider_position < 0.92F);
  TEST_ASSERT_TRUE(right_state.slider_position >= 0.0F);
  TEST_ASSERT_TRUE(right_state.slider_position <= 1.0F);
}

void test_isolated_full_height_bar_becomes_explicit_noise_event() {
  espsand::input::TouchSemanticInterpreter interpreter;
  auto diagnostics = ready_touch();
  diagnostics.common_mode_z = 2.0F;
  activate(diagnostics, 5, 12.0F);

  auto state = interpreter.update(diagnostics);
  TEST_ASSERT_FALSE(state.slider_active);
  TEST_ASSERT_TRUE(state.noise_event);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, state.noise_impulse);

  state = interpreter.update(diagnostics);
  TEST_ASSERT_FALSE(state.noise_event);
  TEST_ASSERT_TRUE(state.noise_impulse > 0.0F);
}

void test_broad_deliberate_slider_contact_is_not_noise() {
  espsand::input::TouchSemanticInterpreter interpreter;
  auto diagnostics = ready_touch();
  diagnostics.common_mode_z = 11.0F;
  activate(diagnostics, 2, 10.0F);
  activate(diagnostics, 3, 11.0F);
  activate(diagnostics, 4, 9.0F);

  const auto state = interpreter.update(diagnostics);
  TEST_ASSERT_TRUE(state.slider_active);
  TEST_ASSERT_FALSE(state.noise_event);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.noise_impulse);
}

void test_not_ready_never_exports_slider_or_noise() {
  espsand::input::TouchSemanticInterpreter interpreter;
  espsand::io::TouchDiagnostics diagnostics{};
  diagnostics.channel_count = 7;
  diagnostics.common_mode_z = 20.0F;
  activate(diagnostics, 0, 20.0F);
  activate(diagnostics, 1, 20.0F);

  const auto state = interpreter.update(diagnostics);
  TEST_ASSERT_FALSE(state.slider_active);
  TEST_ASSERT_FALSE(state.noise_event);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, state.noise_impulse);
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_slider_requires_full_common_mode_and_two_local_channels);
  RUN_TEST(test_slider_centroid_moves_smoothly_across_edge);
  RUN_TEST(test_isolated_full_height_bar_becomes_explicit_noise_event);
  RUN_TEST(test_broad_deliberate_slider_contact_is_not_noise);
  RUN_TEST(test_not_ready_never_exports_slider_or_noise);
  return UNITY_END();
}
