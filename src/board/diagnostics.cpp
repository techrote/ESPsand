#include "diagnostics.hpp"

#include <Arduino.h>

#include <espsand/core/foundation.hpp>

#include "board_profile.hpp"

namespace espsand::board {
namespace {

const char* status_name(EvidenceStatus status) {
  switch (status) {
  case EvidenceStatus::kKnown:
    return "KNOWN";
  case EvidenceStatus::kAssumed:
    return "ASSUMED";
  case EvidenceStatus::kNeedsPhysicalValidation:
    return "NEEDS_PHYSICAL_VALIDATION";
  }
  return "INVALID";
}

void print_pin_fact(const PinFact& fact) {
  Serial.printf("profile.pin signal=%s gpio=%d status=%s\n", fact.signal, fact.gpio,
                status_name(fact.status));
}

} // namespace

void print_foundation_probe() {
  Serial.println("espsand.probe begin");
  Serial.printf("project.name=%s\n", espsand::core::project_name());
  Serial.printf("profile.product_family=%s status=%s\n", kExpectedProductFamily,
                status_name(kProductFamilyStatus));

  Serial.printf("chip.model=%s\n", ESP.getChipModel());
  Serial.printf("chip.revision=%u\n", ESP.getChipRevision());
  Serial.printf("chip.cores=%u\n", ESP.getChipCores());
  Serial.printf("chip.cpu_mhz=%u\n", ESP.getCpuFreqMHz());
  Serial.printf("chip.sdk=%s\n", ESP.getSdkVersion());
  Serial.printf("memory.flash_bytes=%u\n", ESP.getFlashChipSize());
  Serial.printf("memory.psram_bytes=%u\n", ESP.getPsramSize());
  Serial.printf("memory.heap_bytes=%u\n", ESP.getHeapSize());

  print_pin_fact(kMatrixData);
  print_pin_fact(kImuSda);
  print_pin_fact(kImuScl);
  print_pin_fact(kImuInt1);
  print_pin_fact(kImuInt2);
  print_pin_fact(kBootButton);

  Serial.println("profile.matrix_protocol=ASSUMED addressable_rgb");
  Serial.println("profile.matrix_pixel_order=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("profile.matrix_colour_order=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("profile.imu_i2c_address=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("profile.imu_axis_orientation=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("profile.boot_active_level=ASSUMED low");
  Serial.println("profile.touch_candidates=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("profile.brightness_ceiling=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("espsand.probe end");
}

} // namespace espsand::board
