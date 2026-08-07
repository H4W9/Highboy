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

#ifndef BLE_SCREEN_SERVER_H
#define BLE_SCREEN_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Initialize the BLE screen streaming GATT service.
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NO_MEM if allocation fails
 */
esp_err_t ble_screen_server_init(void);

/**
 * @brief Deinitialize the BLE screen streaming service and free resources.
 */
void ble_screen_server_deinit(void);

/**
 * @brief Send a partial screen update over BLE notifications.
 *
 * @param px_map Pointer to pixel data (RGB565). Must not be NULL.
 * @param x      X offset of the partial region.
 * @param y      Y offset of the partial region.
 * @param w      Width of the partial region.
 * @param h      Height of the partial region.
 */
void ble_screen_server_send_partial(const uint16_t *px_map, int x, int y, int w, int h);

/**
 * @brief Check if the screen server is initialized and active.
 *
 * @return true if active, false otherwise.
 */
bool ble_screen_server_is_active(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_SCREEN_SERVER_H
