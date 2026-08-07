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

#ifndef MESHTASTIC_WIFI_H
#define MESHTASTIC_WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Meshtastic WiFi AP + TCP server.
 *
 * Creates a WiFi AP and listens on port 4403 for the Meshtastic app.
 * Uses the same protobuf protocol as BLE (StreamAPI framing).
 *
 * @param node_num  This device's node number.
 * @return ESP_OK on success.
 */
esp_err_t meshtastic_wifi_init(uint32_t node_num);

/**
 * @brief Process incoming TCP connections and data. Call from main loop.
 */
void meshtastic_wifi_poll(void);

/**
 * @brief Check if a client is connected.
 */
bool meshtastic_wifi_is_connected(void);

/**
 * @brief Get the current client socket fd. Returns -1 if not connected.
 */
int meshtastic_wifi_get_client_fd(void);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_WIFI_H
