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
 * @file storage_internal.h
 * @brief Shared utilities for storage_api source files (not public API).
 */

#ifndef STORAGE_INTERNAL_H
#define STORAGE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "vfs_config.h"

/**
 * @brief Resolve a relative or absolute path to a full VFS path.
 *
 * If the path already starts with VFS_MOUNT_POINT, it is copied as-is.
 * If it starts with '/', the mount point is prepended without a separator.
 * Otherwise, mount point + '/' is prepended.
 *
 * @param path       Input path (relative or absolute).
 * @param out_full   Output buffer for the resolved path.
 * @param out_size   Size of the output buffer.
 */
static inline void storage_resolve_path(const char *path, char *out_full, size_t out_size) {
  if (strncmp(path, VFS_MOUNT_POINT, strlen(VFS_MOUNT_POINT)) == 0) {
    snprintf(out_full, out_size, "%s", path);
  } else if (path[0] == '/') {
    snprintf(out_full, out_size, "%s%s", VFS_MOUNT_POINT, path);
  } else {
    snprintf(out_full, out_size, "%s/%s", VFS_MOUNT_POINT, path);
  }
}

#ifdef __cplusplus
}
#endif

#endif // STORAGE_INTERNAL_H
