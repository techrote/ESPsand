#pragma once

#include <cstdint>

#include <espsand/io/interfaces.hpp>

namespace espsand::board {

class SerialDiagnostics final : public io::IDiagnostics {
public:
  void begin(std::uint32_t baud = 115200);
  void write_line(const char* line) override;
};

} // namespace espsand::board
