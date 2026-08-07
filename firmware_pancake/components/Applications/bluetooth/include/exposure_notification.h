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

#ifndef EXPOSURE_NOTIFICATION_H
#define EXPOSURE_NOTIFICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Record for a detected GAEN (Google/Apple Exposure Notification) device.
 */
typedef struct {
  uint8_t addr[6];
  int8_t rssi;
  uint32_t last_seen; /**< Timestamp in seconds */
} exposure_notification_device_t;

/**
 * @brief Start scanning for GAEN exposure notification beacons.
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NO_MEM if allocation fails
 */
esp_err_t exposure_notification_start(void);

/**
 * @brief Stop scanning for exposure notification beacons.
 */
void exposure_notification_stop(void);

/**
 * @brief Get the list of detected GAEN devices.
 *
 * @param[out] out_count Pointer to store the number of devices found.
 * @return Pointer to the device list, or NULL on failure.
 */
exposure_notification_device_t *exposure_notification_get_list(uint16_t *out_count);

/**
 * @brief Get the number of currently detected GAEN devices.
 *
 * @return Number of detected devices.
 */
uint16_t exposure_notification_get_count(void);

/**
 * @brief Reset the detected device list.
 */
void exposure_notification_reset(void);

#ifdef __cplusplus
}
#endif

#endif // EXPOSURE_NOTIFICATION_H
