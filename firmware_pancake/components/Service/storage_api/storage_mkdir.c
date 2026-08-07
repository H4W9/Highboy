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

#include "storage_mkdir.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"

#include "vfs_core.h"

static const char *TAG = "STORAGE_MKDIR";

esp_err_t storage_mkdir_recursive(const char *path) {
  if (path == NULL) {
    ESP_LOGE(TAG, "Path is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGD(TAG, "Creating directory path: %s", path);

  char temp_path[VFS_MAX_PATH];
  strncpy(temp_path, path, sizeof(temp_path) - 1);
  temp_path[sizeof(temp_path) - 1] = '\0';

  // Remove trailing slash if present
  size_t len = strlen(temp_path);
  if (len > 1 && temp_path[len - 1] == '/') {
    temp_path[len - 1] = '\0';
  }

  char build_path[VFS_MAX_PATH] = "";
  char path_copy[VFS_MAX_PATH];
  strncpy(path_copy, temp_path, sizeof(path_copy) - 1);
  path_copy[sizeof(path_copy) - 1] = '\0';

  char *token;
  char *saveptr;
  bool is_absolute = (temp_path[0] == '/');

  token = strtok_r(path_copy, "/", &saveptr);

  while (token != NULL) {
    // Build the path progressively
    if (strlen(build_path) > 0) {
      strcat(build_path, "/");
    } else if (is_absolute) {
      strcat(build_path, "/");
    }
    strcat(build_path, token);

    struct stat st;
    if (stat(build_path, &st) != 0) {
      // Directory doesn't exist, create it
      ESP_LOGI(TAG, "Creating directory: %s", build_path);
      if (mkdir(build_path, 0755) != 0) {
        if (errno == EEXIST) {
          ESP_LOGD(TAG, "Directory already exists: %s", build_path);
        } else {
          ESP_LOGE(TAG,
                   "Failed to create directory: %s (errno: %d - %s)",
                   build_path,
                   errno,
                   strerror(errno));
          return ESP_FAIL;
        }
      } else {
        ESP_LOGI(TAG, "Successfully created: %s", build_path);
      }
    } else {
      // Path exists, check if it's a directory
      if (!S_ISDIR(st.st_mode)) {
        ESP_LOGE(TAG, "Path exists but is not a directory: %s", build_path);
        return ESP_FAIL;
      }
      ESP_LOGD(TAG, "Directory already exists: %s", build_path);
    }

    token = strtok_r(NULL, "/", &saveptr);
  }

  ESP_LOGD(TAG, "Successfully processed path: %s", path);
  return ESP_OK;
}