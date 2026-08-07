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

#ifndef SUBGHZ_STORAGE_H
#define SUBGHZ_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "subghz_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Sub-GHz storage subsystem.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t subghz_storage_init(void);

/**
 * @brief Save a decoded signal to persistent storage.
 *
 * @param name       User-assigned name for the capture.
 * @param data       Decoded signal data.
 * @param frequency  Center frequency in Hz.
 * @param te         Base time element in microseconds.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t subghz_storage_save_decoded(const char *name,
                                      const subghz_data_t *data,
                                      uint32_t frequency,
                                      uint32_t te);

/**
 * @brief Save raw pulse data to persistent storage.
 *
 * @param name       User-assigned name for the capture.
 * @param pulses     Array of signed pulse durations in microseconds.
 * @param count      Number of elements in the pulses array.
 * @param frequency  Center frequency in Hz.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t
subghz_storage_save_raw(const char *name, const int32_t *pulses, size_t count, uint32_t frequency);

#ifdef __cplusplus
}
#endif

#endif // SUBGHZ_STORAGE_H
