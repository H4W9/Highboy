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

#ifndef DUCKY_PARSER_H
#define DUCKY_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Keyboard layout identifiers.
 */
typedef enum { DUCKY_LAYOUT_US = 0, DUCKY_LAYOUT_ABNT2, DUCKY_LAYOUT_COUNT } ducky_layout_t;

/**
 * @brief Output transport mode.
 */
typedef enum {
  DUCKY_OUTPUT_USB = 0,
  DUCKY_OUTPUT_BLUETOOTH,
  DUCKY_OUTPUT_COUNT
} ducky_output_mode_t;

/**
 * @brief Progress callback invoked after each line is executed.
 *
 * @param current_line  Current line number (1-based).
 * @param total_lines   Total number of lines in the script.
 */
typedef void (*ducky_progress_cb_t)(int current_line, int total_lines);

/**
 * @brief Set the output transport mode (USB or Bluetooth).
 *
 * @param mode  Transport mode.
 */
void ducky_set_output_mode(ducky_output_mode_t mode);

/**
 * @brief Set the keyboard layout for STRING commands.
 *
 * @param layout  Keyboard layout to use.
 */
void ducky_set_layout(ducky_layout_t layout);

/**
 * @brief Register a progress callback for script execution.
 *
 * @param cb  Callback function, or NULL to disable.
 */
void ducky_set_progress_callback(ducky_progress_cb_t cb);

/**
 * @brief Parse and execute a DuckyScript string.
 *
 * Executes each line sequentially. Can be aborted via ducky_abort().
 *
 * @param script  Null-terminated DuckyScript string.
 */
void ducky_parse_and_run(const char *script);

/**
 * @brief Load and execute a DuckyScript file from internal assets.
 *
 * @param filename  Asset filename (without path prefix).
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NOT_FOUND if the file does not exist
 *   - ESP_ERR_NO_MEM if allocation fails
 */
esp_err_t ducky_run_from_assets(const char *filename);

/**
 * @brief Load and execute a DuckyScript file from the SD card.
 *
 * @param path  Full path to the script file on the SD card.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NOT_FOUND if the file does not exist
 *   - ESP_ERR_NO_MEM if allocation fails
 */
esp_err_t ducky_run_from_sdcard(const char *path);

/**
 * @brief Abort the currently running script.
 *
 * Sets an internal flag that is checked between lines. The script
 * will stop at the next line boundary.
 */
void ducky_abort(void);

#ifdef __cplusplus
}
#endif

#endif // DUCKY_PARSER_H
