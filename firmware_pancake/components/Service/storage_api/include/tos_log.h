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

#ifndef TOS_LOG_H
#define TOS_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Initialize the persistent log system.
 *
 * Redirects all ESP_LOGx output to a file on the SD card via
 * esp_log_set_vprintf. Logs are written to both serial and file
 * simultaneously.
 *
 * Log files are rotated automatically:
 *   - sys.1.log (current) -> sys.2.log -> ... -> sys.5.log
 *   - Maximum 5 files, total limited to 10 MB
 *   - When sys.1.log exceeds 2 MB, rotation occurs
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_FAIL if the log file cannot be opened
 */
esp_err_t tos_log_init(void);

/**
 * @brief Flush and close the log file.
 *
 * Restores the default ESP_LOG vprintf handler. Call this before
 * unmounting the SD card.
 */
void tos_log_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // TOS_LOG_H
