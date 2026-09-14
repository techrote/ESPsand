#pragma once

#include <cstdint>

#include "types.hpp"

namespace espsand::io {

class IClock {
public:
  virtual ~IClock() = default;
  virtual std::uint64_t now_us() const = 0;
};

class IImu {
public:
  virtual ~IImu() = default;
  virtual bool poll(ImuSample& sample) = 0;
  virtual ImuStatus status() const = 0;
};

class IButton {
public:
  virtual ~IButton() = default;
  virtual ButtonEvent poll(std::uint64_t now_us) = 0;
};

class IMatrixOutput {
public:
  virtual ~IMatrixOutput() = default;
  virtual void present(const Frame8x8& frame, std::uint8_t requested_brightness) = 0;
  virtual std::uint8_t brightness_ceiling() const = 0;
};

class IDiagnostics {
public:
  virtual ~IDiagnostics() = default;
  virtual void write_line(const char* line) = 0;
};

} // namespace espsand::io
