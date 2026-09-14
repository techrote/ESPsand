#include <espsand/input/button_gesture.hpp>

namespace espsand::input {

ButtonGesture::ButtonGesture(ButtonGestureConfig config) : config_(config) {}

void ButtonGesture::reset(bool initial_pressed, std::uint32_t now_ms) {
  initialized_ = true;
  raw_pressed_ = initial_pressed;
  stable_pressed_ = initial_pressed;
  long_emitted_ = false;
  raw_changed_ms_ = now_ms;
  pressed_since_ms_ = now_ms;
}

io::ButtonEvent ButtonGesture::update(bool raw_pressed, std::uint32_t now_ms) {
  if (!initialized_) {
    reset(raw_pressed, now_ms);
    return io::ButtonEvent::kNone;
  }

  if (raw_pressed != raw_pressed_) {
    raw_pressed_ = raw_pressed;
    raw_changed_ms_ = now_ms;
  }

  io::ButtonEvent event = io::ButtonEvent::kNone;
  if (raw_pressed_ != stable_pressed_ && elapsed(now_ms, raw_changed_ms_) >= config_.debounce_ms) {
    stable_pressed_ = raw_pressed_;
    if (stable_pressed_) {
      pressed_since_ms_ = now_ms;
      long_emitted_ = false;
    } else {
      const std::uint32_t duration_ms = elapsed(now_ms, pressed_since_ms_);
      if (!long_emitted_ && duration_ms <= config_.short_max_ms) {
        event = io::ButtonEvent::kShortPress;
      }
    }
  }

  if (stable_pressed_ && !long_emitted_ &&
      elapsed(now_ms, pressed_since_ms_) >= config_.long_min_ms) {
    long_emitted_ = true;
    return io::ButtonEvent::kLongPress;
  }

  return event;
}

bool ButtonGesture::stable_pressed() const {
  return stable_pressed_;
}

std::uint32_t ButtonGesture::elapsed(std::uint32_t now, std::uint32_t then) {
  return now - then;
}

} // namespace espsand::input
