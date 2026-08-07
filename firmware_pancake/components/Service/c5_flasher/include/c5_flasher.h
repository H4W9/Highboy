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

#ifndef C5_FLASHER_H
#define C5_FLASHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Initialize UART and GPIO pins for C5 flashing.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t c5_flasher_init(void);

/**
 * @brief Put the C5 into bootloader mode via GPIO strapping.
 */
void c5_flasher_enter_bootloader(void);

/**
 * @brief Reset the C5 into normal operation mode.
 */
void c5_flasher_reset_normal(void);

/**
 * @brief Flash the C5 firmware over UART using ESP serial protocol.
 *
 * If bin_data is NULL and C5_FIRMWARE_EMBEDDED is defined, uses the
 * embedded binary linked at build time.
 *
 * @param bin_data  Pointer to firmware binary, or NULL to use embedded.
 * @param bin_size  Size of the binary in bytes.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t c5_flasher_update(const uint8_t *bin_data, uint32_t bin_size);

#ifdef __cplusplus
}
#endif

#endif // C5_FLASHER_H
