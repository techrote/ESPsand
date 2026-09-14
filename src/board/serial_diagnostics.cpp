#include "serial_diagnostics.hpp"

#include <Arduino.h>

namespace espsand::board {

void SerialDiagnostics::begin(std::uint32_t baud) { Serial.begin(baud); }

void SerialDiagnostics::write_line(const char* line) { Serial.println(line); }

} // namespace espsand::board
