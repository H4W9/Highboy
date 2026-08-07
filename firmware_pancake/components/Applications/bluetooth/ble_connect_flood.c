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

#include "ble_connect_flood.h"

#include <string.h>

#include "esp_log.h"

#include "spi_bridge.h"
#include "spi_session.h"

static const char *TAG = "BLE_CONNECT_FLOOD";

#define FLOOD_ADDR_PAYLOAD_SIZE 7

static bool s_is_running = false;
static uint32_t s_session_id = SPI_SESSION_INVALID_ID;

esp_err_t ble_connect_flood_start(const uint8_t *addr, uint8_t addr_type) {
  if (addr == NULL)
    return ESP_ERR_INVALID_ARG;
  uint8_t payload[FLOOD_ADDR_PAYLOAD_SIZE];
  memcpy(payload, addr, 6);
  payload[6] = addr_type;
  s_session_id = spi_session_start(SPI_ID_BT_APP_FLOOD, payload, sizeof(payload), NULL, NULL);
  s_is_running = (s_session_id != SPI_SESSION_INVALID_ID);
  if (!s_is_running) {
    ESP_LOGW(TAG, "BLE flood failed over SPI.");
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t ble_connect_flood_stop(void) {
  if (s_session_id != SPI_SESSION_INVALID_ID) {
    spi_session_stop(s_session_id);
    s_session_id = SPI_SESSION_INVALID_ID;
  }
  s_is_running = false;
  return ESP_OK;
}

bool ble_connect_flood_is_running(void) {
  return s_is_running;
}
