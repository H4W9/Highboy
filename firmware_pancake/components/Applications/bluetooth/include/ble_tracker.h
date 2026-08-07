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

#ifndef BLE_TRACKER_H
#define BLE_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Start tracking a BLE device by address, monitoring its RSSI.
 *
 * @param addr Target device BLE address (6 bytes).
 * @return
 *   - ESP_OK on success
 */
esp_err_t ble_tracker_start(const uint8_t *addr);

/**
 * @brief Stop tracking the BLE device.
 */
void ble_tracker_stop(void);

/**
 * @brief Get the latest RSSI value from the tracked device.
 *
 * @return RSSI value in dBm, or -120 if no reading available.
 */
int ble_tracker_get_rssi(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_TRACKER_H
