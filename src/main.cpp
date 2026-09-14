#include <Arduino.h>

#ifndef ESPSAND_MINIMAL_BRINGUP

#include "app/diagnostic_runtime.hpp"

namespace {

espsand::app::DiagnosticRuntime runtime;

} // namespace

void setup() {
  runtime.begin();
}

void loop() {
  runtime.tick();
  yield();
}

#endif // ESPSAND_MINIMAL_BRINGUP
