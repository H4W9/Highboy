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
 * @file vfs_littlefs.h
 * @brief LittleFS backend interface.
 */
#ifndef VFS_LITTLEFS_H
#define VFS_LITTLEFS_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t vfs_littlefs_init(void);
esp_err_t vfs_littlefs_deinit(void);

bool vfs_littlefs_is_mounted(void);
void vfs_littlefs_print_info(void);

esp_err_t vfs_littlefs_format(void);
esp_err_t vfs_register_littlefs_backend(void);
esp_err_t vfs_unregister_littlefs_backend(void);

#ifdef __cplusplus
}
#endif

#endif // VFS_LITTLEFS_H
