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

#ifndef BAD_USB_H
#define BAD_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Initialize the BadUSB module.
 *
 * Initializes the TinyUSB stack and registers the USB HID driver
 * into the HID HAL layer.
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_STATE if already initialized
 */
esp_err_t bad_usb_init(void);

/**
 * @brief Deinitialize the BadUSB module.
 *
 * Unregisters the HID callbacks and uninstalls the TinyUSB driver.
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_STATE if not initialized
 */
esp_err_t bad_usb_deinit(void);

/**
 * @brief Block until the USB host mounts the device.
 *
 * Polls tud_mounted() in a loop, then waits an additional 2 seconds
 * to allow the host OS to enumerate the device.
 */
void bad_usb_wait_for_connection(void);

#ifdef __cplusplus
}
#endif

#endif // BAD_USB_H
