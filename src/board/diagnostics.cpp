#include "diagnostics.hpp"

#include <Arduino.h>

#include <espsand/core/foundation.hpp>

#include "board_profile.hpp"
#include "power_policy.hpp"

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

  const auto& power = power_policy_status();
  Serial.printf(
      "power.policy applied=%u wifi_off=%u bt_off=%u bt_mem_released=%u wifi_err=%d bt_err=%d\n",
      power.applied ? 1U : 0U, power.wifi_off ? 1U : 0U, power.bluetooth_off ? 1U : 0U,
      power.bluetooth_memory_released ? 1U : 0U, power.wifi_error, power.bluetooth_error);
  Serial.println("power.keep usb_serial_jtag=on imu=on watchdogs=on brownout=on psram=on");

  print_pin_fact(kMatrixData);
  print_pin_fact(kImuSda);
  print_pin_fact(kImuScl);
  print_pin_fact(kImuInt1);
  print_pin_fact(kImuInt2);
  print_pin_fact(kBootButton);
  print_pin_fact(kUsbDm);
  print_pin_fact(kUsbDp);
  for (const auto& candidate : kTouchCandidates) {
    print_pin_fact(candidate);
  }

  Serial.println("profile.matrix_protocol=KNOWN addressable_rgb");
  Serial.println("profile.matrix_pixel_order=KNOWN row_major_top_left");
  Serial.println("profile.matrix_colour_order=KNOWN RGB");
  Serial.println("profile.imu_i2c_address=KNOWN 0x6B");
  Serial.println("profile.imu_axis_orientation=CALIBRATED_IN_PLANE x=-raw_y y=+raw_x");
  Serial.println("profile.boot_active_level=KNOWN low");
  Serial.println("profile.touch_candidates=KNOWN gpio=1,2,3,4,5,6,7");
  Serial.println("profile.touch_zones=KNOWN combo_only_common_mode local_ab=disabled");
  Serial.println("profile.brightness_ceiling=NEEDS_PHYSICAL_VALIDATION");
  Serial.println("espsand.probe end");
}

} // namespace espsand::board
