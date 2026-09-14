#pragma once

#include "interfaces.hpp"

namespace espsand::io {

class NullTouchZones final : public ITouchZones {
public:
  bool poll(std::uint64_t now_us, TouchFrame& frame) override {
    frame = {};
    frame.timestamp_us = now_us;
    return false;
  }

  TouchStatus status() const override {
    return {};
  }

  TouchDiagnostics diagnostics() const override {
    return {};
  }
};

} // namespace espsand::io
