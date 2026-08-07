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

/**
 * @file ft6336.h
 * @brief FT6336 / FT6236 / FT6436 capacitive touch driver (Pancake, I2C).
 *
 * Register-level logic ported from the M5PORKCHOP pancake port
 * (src/pancake/FT6336Touch.h) to ESP-IDF C. Uses the legacy I2C master
 * bus on I2C_NUM_0 already initialized by init_i2c().
 */

#ifndef FT6336_H
#define FT6336_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Reset and probe the FT6336 controller.
 *
 * Pulses the touch reset line and reads the vendor ID register.
 * The I2C bus must already be initialized (init_i2c()).
 *
 * @return ESP_OK if the controller responded, ESP_FAIL otherwise.
 */
esp_err_t ft6336_init(void);

/**
 * @brief Read the current touch point, mapped into display coordinates.
 *
 * @param[out] x  X coordinate in display space [0, LCD_H_RES).
 * @param[out] y  Y coordinate in display space [0, LCD_V_RES).
 * @return true if a finger is currently down, false otherwise.
 */
bool ft6336_get_point(int16_t *x, int16_t *y);

#ifdef __cplusplus
}
#endif

#endif // FT6336_H
