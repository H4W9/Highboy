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

#ifndef STORAGE_MKDIR_H
#define STORAGE_MKDIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Create a directory path recursively (like mkdir -p).
 *
 * Creates each component of the path that does not already exist.
 * Existing directories are silently skipped. Fails if any path
 * component exists but is not a directory.
 *
 * @param path  Absolute path to create.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if path is NULL.
 */
esp_err_t storage_mkdir_recursive(const char *path);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_MKDIR_H
