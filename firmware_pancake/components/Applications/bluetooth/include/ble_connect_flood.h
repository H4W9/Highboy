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

#ifndef BLE_CONNECT_FLOOD_H
#define BLE_CONNECT_FLOOD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Start BLE connection flood attack against a target device.
 *
 * @param addr      Target device BLE address (6 bytes).
 * @param addr_type Target device address type.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_STATE if already running
 *   - ESP_FAIL if bluetooth service is not running
 *   - ESP_ERR_NO_MEM if allocation fails
 */
esp_err_t ble_connect_flood_start(const uint8_t *addr, uint8_t addr_type);

/**
 * @brief Stop the BLE connection flood attack.
 *
 * @return
 *   - ESP_OK on success
 */
esp_err_t ble_connect_flood_stop(void);

/**
 * @brief Check if the connection flood is currently running.
 *
 * @return true if running, false otherwise.
 */
bool ble_connect_flood_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_CONNECT_FLOOD_H
