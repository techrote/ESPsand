#include "power_policy.hpp"

#include <esp_err.h>
#include <esp_wifi.h>
#include <sdkconfig.h>

#if CONFIG_BT_ENABLED
#include <esp_bt.h>
#endif

namespace espsand::board {
namespace {

PowerPolicyStatus g_status{};

bool stop_wifi(int& error_out) {
  wifi_mode_t mode = WIFI_MODE_NULL;
  const esp_err_t query_result = esp_wifi_get_mode(&mode);

  if (query_result == ESP_ERR_WIFI_NOT_INIT) {
    error_out = ESP_OK;
    return true;
  }
  if (query_result != ESP_OK) {
    error_out = query_result;
    return false;
  }

  const esp_err_t stop_result = esp_wifi_stop();
  if (stop_result != ESP_OK && stop_result != ESP_ERR_WIFI_NOT_STARTED) {
    error_out = stop_result;
    return false;
  }

  const esp_err_t deinit_result = esp_wifi_deinit();
  if (deinit_result == ESP_OK || deinit_result == ESP_ERR_WIFI_NOT_INIT) {
    error_out = ESP_OK;
    return true;
  }

  error_out = deinit_result;
  return false;
}

#if CONFIG_BT_ENABLED
bool stop_bluetooth(bool& memory_released, int& error_out) {
  esp_bt_controller_status_t state = esp_bt_controller_get_status();

  if (state == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    const esp_err_t disable_result = esp_bt_controller_disable();
    if (disable_result != ESP_OK) {
      error_out = disable_result;
      return false;
    }
    state = esp_bt_controller_get_status();
  }

  if (state == ESP_BT_CONTROLLER_STATUS_INITED) {
    const esp_err_t deinit_result = esp_bt_controller_deinit();
    if (deinit_result != ESP_OK) {
      error_out = deinit_result;
      return false;
    }
    state = esp_bt_controller_get_status();
  }

  if (state != ESP_BT_CONTROLLER_STATUS_IDLE) {
    error_out = ESP_ERR_INVALID_STATE;
    return false;
  }

  const esp_err_t release_result = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
  if (release_result != ESP_OK) {
    error_out = release_result;
    return false;
  }

  memory_released = true;
  error_out = ESP_OK;
  return true;
}
#endif

} // namespace

void apply_power_policy() {
  if (g_status.applied) {
    return;
  }

  g_status.applied = true;
  g_status.wifi_off = stop_wifi(g_status.wifi_error);

#if CONFIG_BT_ENABLED
  g_status.bluetooth_off =
      stop_bluetooth(g_status.bluetooth_memory_released, g_status.bluetooth_error);
#else
  g_status.bluetooth_off = true;
  g_status.bluetooth_memory_released = true;
  g_status.bluetooth_error = ESP_OK;
#endif
}

const PowerPolicyStatus& power_policy_status() {
  return g_status;
}

} // namespace espsand::board
