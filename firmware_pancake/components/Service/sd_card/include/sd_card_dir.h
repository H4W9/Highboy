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
 * @file sd_card_dir.h
 * @brief SD card directory operations.
 */

#ifndef SD_CARD_DIR_H
#define SD_CARD_DIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Callback invoked for each directory entry.
 *
 * @param name      Entry name.
 * @param is_dir    true if the entry is a directory.
 * @param user_data User context pointer.
 */
typedef void (*sd_dir_callback_t)(const char *name, bool is_dir, void *user_data);

/**
 * @brief Create a directory (including parents).
 *
 * @param path  Directory path.
 * @return ESP_OK on success.
 */
esp_err_t sd_dir_create(const char *path);

/**
 * @brief Recursively remove a directory and its contents.
 *
 * @param path  Directory path.
 * @return ESP_OK on success.
 */
esp_err_t sd_dir_remove_recursive(const char *path);

/**
 * @brief Check if a directory exists.
 *
 * @param path  Directory path.
 * @return true if the directory exists.
 */
bool sd_dir_exists(const char *path);

/**
 * @brief List directory contents via callback.
 *
 * @param path       Directory path.
 * @param callback   Function called for each entry.
 * @param user_data  User context passed to callback.
 * @return ESP_OK on success.
 */
esp_err_t sd_dir_list(const char *path, sd_dir_callback_t callback, void *user_data);

/**
 * @brief Count files and subdirectories in a directory.
 *
 * @param path            Directory path.
 * @param[out] file_count Number of files found.
 * @param[out] dir_count  Number of subdirectories found.
 * @return ESP_OK on success.
 */
esp_err_t sd_dir_count(const char *path, uint32_t *file_count, uint32_t *dir_count);

/**
 * @brief Recursively copy a directory tree.
 *
 * @param src  Source directory path.
 * @param dst  Destination directory path.
 * @return ESP_OK on success.
 */
esp_err_t sd_dir_copy_recursive(const char *src, const char *dst);

/**
 * @brief Calculate total size of all files in a directory tree.
 *
 * @param path             Directory path.
 * @param[out] total_size  Sum of all file sizes in bytes.
 * @return ESP_OK on success.
 */
esp_err_t sd_dir_get_size(const char *path, uint64_t *total_size);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_DIR_H
