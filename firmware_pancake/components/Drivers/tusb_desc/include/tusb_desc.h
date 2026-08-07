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

#ifndef TUSB_DESC_H
#define TUSB_DESC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "tinyusb.h"

#define TUSB_DESC_ITF_NUM_HID 0

/**
 * @brief Initialize the TinyUSB driver with HID composite descriptors.
 *
 * Installs the GPIO ISR service (required for ESP32-P4 High Speed USB),
 * configures device/configuration/HID report descriptors, and installs
 * the TinyUSB driver.
 *
 * Must be called before any HID report transmission.
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_FAIL if the TinyUSB driver installation fails
 */
esp_err_t busb_init(void);

#ifdef __cplusplus
}
#endif

#endif // TUSB_DESC_H
