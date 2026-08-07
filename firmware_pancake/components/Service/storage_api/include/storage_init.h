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
 * @file storage_init.h
 * @brief Storage initialization — backend agnostic.
 *
 * Automatically initializes the backend selected in vfs_config.h.
 * Supports: SD Card, SPIFFS, LittleFS, RAMFS.
 */

#ifndef STORAGE_INIT_H
#define STORAGE_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Initialize the storage backend with default settings.
 *
 * @return ESP_OK on success, or if already initialized.
 */
esp_err_t storage_init(void);

/**
 * @brief Initialize the storage backend with custom parameters.
 *
 * @param max_files         Maximum number of open files.
 * @param is_format_on_fail  Format the partition if mount fails.
 * @return ESP_OK on success.
 */
esp_err_t storage_init_custom(uint8_t max_files, bool is_format_on_fail);

/**
 * @brief Deinitialize the storage backend and free resources.
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if not initialized.
 */
esp_err_t storage_deinit(void);

/**
 * @brief Check if the storage backend is currently mounted.
 *
 * @return true if mounted and ready.
 */
bool storage_is_mounted(void);

/**
 * @brief Remount the storage (deinit + init).
 *
 * @return ESP_OK on success.
 */
esp_err_t storage_remount(void);

/**
 * @brief Perform a write/read health check on the storage.
 *
 * Creates a temporary file, verifies read-back, and removes it.
 *
 * @return ESP_OK if healthy, ESP_FAIL on read/write error.
 */
esp_err_t storage_check_health(void);

/**
 * @brief Get the VFS mount point path.
 *
 * @return Null-terminated mount point string (e.g. "/sdcard").
 */
const char *storage_get_mount_point(void);

/**
 * @brief Get the backend name.
 *
 * @return Null-terminated backend name (e.g. "SD Card").
 */
const char *storage_get_backend_name(void);

/**
 * @brief Print storage info to the log.
 */
void storage_print_info(void);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_INIT_H
