#include <Arduino.h>

#ifndef ESPSAND_MINIMAL_BRINGUP

#include "app/diagnostic_runtime.hpp"
#include "board/power_policy.hpp"

namespace {

espsand::app::DiagnosticRuntime runtime;

} // namespace

void setup() {
  espsand::board::apply_power_policy();
  runtime.begin();
}

void loop() {
  runtime.tick();
  yield();
}

#endif // ESPSAND_MINIMAL_BRINGUP
