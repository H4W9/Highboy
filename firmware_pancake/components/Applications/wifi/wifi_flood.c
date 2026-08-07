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

#include "wifi_flood.h"

#include <string.h>

#include "esp_log.h"

#include "spi_bridge.h"
#include "spi_session.h"

static const char *TAG = "WIFI_FLOOD";

#define WIFI_FLOOD_TYPE_AUTH  0
#define WIFI_FLOOD_TYPE_ASSOC 1
#define WIFI_FLOOD_TYPE_PROBE 2

static bool s_is_running = false;
static uint32_t s_session_id = SPI_SESSION_INVALID_ID;

static bool start_flood(uint8_t type, const uint8_t *target_bssid, uint8_t channel) {
  uint8_t payload[8];
  payload[0] = type;
  if (target_bssid != NULL)
    memcpy(payload + 1, target_bssid, 6);
  else
    memset(payload + 1, 0xFF, 6);
  payload[7] = channel;
  s_session_id = spi_session_start(SPI_ID_WIFI_APP_FLOOD, payload, sizeof(payload), NULL, NULL);
  s_is_running = (s_session_id != SPI_SESSION_INVALID_ID);
  return s_is_running;
}

bool wifi_flood_auth_start(const uint8_t *target_bssid, uint8_t channel) {
  if (!start_flood(WIFI_FLOOD_TYPE_AUTH, target_bssid, channel))
    ESP_LOGW(TAG, "Wi-Fi auth flood failed over SPI");
  return s_is_running;
}

bool wifi_flood_assoc_start(const uint8_t *target_bssid, uint8_t channel) {
  if (!start_flood(WIFI_FLOOD_TYPE_ASSOC, target_bssid, channel))
    ESP_LOGW(TAG, "Wi-Fi assoc flood failed over SPI");
  return s_is_running;
}

bool wifi_flood_probe_start(const uint8_t *target_bssid, uint8_t channel) {
  if (!start_flood(WIFI_FLOOD_TYPE_PROBE, target_bssid, channel))
    ESP_LOGW(TAG, "Wi-Fi probe flood failed over SPI");
  return s_is_running;
}

void wifi_flood_stop(void) {
  if (s_session_id != SPI_SESSION_INVALID_ID) {
    spi_session_stop(s_session_id);
    s_session_id = SPI_SESSION_INVALID_ID;
  }
  s_is_running = false;
}

bool wifi_flood_is_running(void) {
  return s_is_running;
}
