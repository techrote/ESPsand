#include <Arduino.h>

#ifdef ESPSAND_MINIMAL_BRINGUP

namespace {

constexpr std::uint8_t kNeoPixelPin = 14;
constexpr std::uint8_t kProbeBrightness = 8;
constexpr unsigned long kPhasePeriodMs = 1000;

unsigned long last_phase_ms = 0;
std::uint8_t phase = 0;

void show_phase(std::uint8_t value) {
  switch (value % 4U) {
  case 0:
    neopixelWrite(kNeoPixelPin, kProbeBrightness, 0, 0);
    break;
  case 1:
    neopixelWrite(kNeoPixelPin, 0, kProbeBrightness, 0);
    break;
  case 2:
    neopixelWrite(kNeoPixelPin, 0, 0, kProbeBrightness);
    break;
  default:
    neopixelWrite(kNeoPixelPin, 0, 0, 0);
    break;
  }
}

} // namespace

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // This target exists only to isolate user-code execution, native USB CDC,
  // and Waveshare's vendor-documented GPIO14 NeoPixel path. A short startup
  // delay gives the host a chance to settle after the upload/reset cycle.
  delay(750);

  Serial.println("espsand.bringup boot");
  Serial.println("espsand.bringup gpio14=vendor_neopixel serial=hwcdc");

  show_phase(phase);
  last_phase_ms = millis();
}

void loop() {
  const unsigned long now_ms = millis();
  if (now_ms - last_phase_ms >= kPhasePeriodMs) {
    last_phase_ms = now_ms;
    phase = static_cast<std::uint8_t>((phase + 1U) % 4U);
    show_phase(phase);
    Serial.printf("espsand.bringup alive ms=%lu phase=%u\n", now_ms, phase);
  }

  yield();
}

#endif // ESPSAND_MINIMAL_BRINGUP
