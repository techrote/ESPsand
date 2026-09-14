#include <Arduino.h>

#include "app/diagnostic_runtime.hpp"

namespace {

espsand::app::DiagnosticRuntime runtime;

} // namespace

void setup() { runtime.begin(); }

void loop() {
  runtime.tick();
  yield();
}
