// Copyright (c) 2025 HIGH CODE LLC
//
// TentacleOS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TentacleOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TentacleOS. If not, see <https://www.gnu.org/licenses/>.

#include "ble_l2cap_flood.h"

#include <string.h>

#include "esp_log.h"

#include "bluetooth_service.h"
#include "spi_bridge.h"
#include "spi_protocol.h"
#include "spi_session.h"

static const char *TAG = "BLE_L2CAP_FLOOD";

#define L2CAP_ADDR_PAYLOAD_SIZE 7

static bool s_is_running = false;
static uint32_t s_session_id = SPI_SESSION_INVALID_ID;

esp_err_t ble_l2cap_flood_start(const uint8_t *addr, uint8_t addr_type) {
  if (s_is_running) {
    return ESP_ERR_INVALID_STATE;
  }

  if (!bluetooth_service_is_running()) {
    ESP_LOGE(TAG, "Bluetooth service not running");
    return ESP_FAIL;
  }

  uint8_t payload[L2CAP_ADDR_PAYLOAD_SIZE];
  memcpy(payload, addr, 6);
  payload[6] = addr_type;

  s_session_id = spi_session_start(SPI_ID_BT_APP_FLOOD, payload, sizeof(payload), NULL, NULL);
  if (s_session_id == SPI_SESSION_INVALID_ID) {
    ESP_LOGE(TAG, "Failed to start L2CAP flood on C5");
    return ESP_FAIL;
  }

  s_is_running = true;
  ESP_LOGI(TAG, "L2CAP Flood started");
  return ESP_OK;
}

esp_err_t ble_l2cap_flood_stop(void) {
  if (!s_is_running)
    return ESP_OK;

  if (s_session_id != SPI_SESSION_INVALID_ID) {
    spi_session_stop(s_session_id);
    s_session_id = SPI_SESSION_INVALID_ID;
  }
  s_is_running = false;
  ESP_LOGI(TAG, "L2CAP Flood stopped");
  return ESP_OK;
}

bool ble_l2cap_flood_is_running(void) {
  return s_is_running;
}
