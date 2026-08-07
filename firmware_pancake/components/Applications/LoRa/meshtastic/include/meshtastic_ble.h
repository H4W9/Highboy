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

#ifndef MESHTASTIC_BLE_H
#define MESHTASTIC_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief NimBLE peripheral implementing the Meshtastic GATT service.
 *
 * Exposes four characteristics under Service UUID
 * 6ba1b218-15a8-461f-9fa8-5dcae273eafd:
 *   - TORADIO   (WRITE)       - app to device
 *   - FROMRADIO (READ)        - device to app (polling)
 *   - FROMNUM   (READ|NOTIFY) - wake signal for the app
 *   - LOGRADIO  (READ|NOTIFY) - optional log stream
 *
 * Plus the standard Battery Service (0x180F / 0x2A19).
 *
 * Pairing uses Secure Connections with MITM + bonding and display-only
 * I/O capability (RANDOM_PIN mode). A fresh 6-digit passkey is
 * generated per pairing and logged at ESP_LOGI.
 *
 * All protocol decisions are delegated to meshtastic_phoneapi - this
 * module only owns the BLE stack lifecycle and GATT plumbing.
 */

/**
 * @brief Initialize NimBLE, register GATT services, start the host task.
 *
 * @param node_num  Our node number.
 * @return ESP_OK on success, esp_err_t from NimBLE otherwise.
 */
esp_err_t meshtastic_ble_init(uint32_t node_num);

/**
 * @brief Start advertising. Called internally on init and after disconnect.
 */
esp_err_t meshtastic_ble_start(void);

/**
 * @brief Stop advertising.
 */
void meshtastic_ble_stop(void);

/**
 * @brief Whether an app is currently bonded and connected.
 */
bool meshtastic_ble_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_BLE_H
