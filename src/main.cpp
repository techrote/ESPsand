#include <Arduino.h>

#include "board/diagnostics.hpp"

namespace {

constexpr unsigned long kSerialBaud = 115200;
constexpr unsigned long kHeartbeatPeriodMs = 5000;
constexpr unsigned long kProbeRepeatPeriodMs = 30000;

unsigned long last_heartbeat_ms = 0;
unsigned long last_probe_ms = 0;

} // namespace

void setup() {
  Serial.begin(kSerialBaud);
  espsand::board::print_foundation_probe();
}

void loop() {
  const unsigned long now = millis();

  if (now - last_heartbeat_ms >= kHeartbeatPeriodMs) {
    last_heartbeat_ms = now;
    Serial.printf("espsand.alive ms=%lu free_heap=%u\n", now, ESP.getFreeHeap());
  }

  if (now - last_probe_ms >= kProbeRepeatPeriodMs) {
    last_probe_ms = now;
    espsand::board::print_foundation_probe();
  }

  yield();
}
