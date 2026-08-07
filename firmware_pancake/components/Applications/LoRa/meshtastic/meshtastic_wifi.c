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

#include "meshtastic_wifi.h"

#include "esp_log.h"

#include "meshtastic_phone_bridge.h"

static const char *TAG = "MESH_WIFI";

esp_err_t meshtastic_wifi_init(uint32_t node_num) {
  (void)node_num;
  esp_err_t ret = meshtastic_phone_bridge_wifi_start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(ret));
  }
  return ret;
}

void meshtastic_wifi_poll(void) {}

bool meshtastic_wifi_is_connected(void) {
  return meshtastic_phone_bridge_is_connected();
}

int meshtastic_wifi_get_client_fd(void) {
  return -1;
}
