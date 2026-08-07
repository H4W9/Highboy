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
 * @file vfs_auto.c
 * @brief Automatic VFS backend initialization
 */

#include "vfs_config.h"

#include <inttypes.h>

#include "esp_log.h"

#include "vfs_core.h"

static const char *TAG = "VFS_AUTO";

static bool s_is_backend_mounted = false;

#ifdef VFS_USE_SD_CARD
extern esp_err_t vfs_register_sd_backend(void);
extern esp_err_t vfs_unregister_sd_backend(void);
extern bool vfs_sdcard_is_mounted(void);
#endif

#ifdef VFS_USE_SPIFFS
extern esp_err_t vfs_register_spiffs_backend(void);
extern esp_err_t vfs_unregister_spiffs_backend(void);
#endif

#ifdef VFS_USE_LITTLEFS
extern esp_err_t vfs_register_littlefs_backend(void);
extern esp_err_t vfs_unregister_littlefs_backend(void);
extern bool vfs_littlefs_is_mounted(void);
#endif

#ifdef VFS_USE_RAMFS
extern esp_err_t vfs_register_ramfs_backend(void);
extern esp_err_t vfs_unregister_ramfs_backend(void);
#endif

esp_err_t vfs_init_auto(void) {
  ESP_LOGI(TAG, "Initializing backend: %s", VFS_BACKEND_NAME);
  ESP_LOGI(TAG, "Mount point: %s", VFS_MOUNT_POINT);

  esp_err_t ret = ESP_FAIL;

#ifdef VFS_USE_SD_CARD
  ret = vfs_register_sd_backend();
  if (ret == ESP_OK) {
    s_is_backend_mounted = true;
  }
#endif

#ifdef VFS_USE_SPIFFS
  ret = vfs_register_spiffs_backend();
  if (ret == ESP_OK) {
    s_is_backend_mounted = true;
  }
#endif

#ifdef VFS_USE_LITTLEFS
  ret = vfs_register_littlefs_backend();
  if (ret == ESP_OK) {
    s_is_backend_mounted = true;
  }
#endif

#ifdef VFS_USE_RAMFS
  ret = vfs_register_ramfs_backend();
  if (ret == ESP_OK) {
    s_is_backend_mounted = true;
  }
#endif

  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Backend %s initialized successfully", VFS_BACKEND_NAME);
  } else {
    ESP_LOGE(TAG, "Failed to initialize backend %s", VFS_BACKEND_NAME);
    s_is_backend_mounted = false;
  }

  return ret;
}

esp_err_t vfs_deinit_auto(void) {
  ESP_LOGI(TAG, "Deinitializing backend: %s", VFS_BACKEND_NAME);

  esp_err_t ret = ESP_FAIL;

#ifdef VFS_USE_SD_CARD
  ret = vfs_unregister_sd_backend();
#endif

#ifdef VFS_USE_SPIFFS
  ret = vfs_unregister_spiffs_backend();
#endif

#ifdef VFS_USE_LITTLEFS
  ret = vfs_unregister_littlefs_backend();
#endif

#ifdef VFS_USE_RAMFS
  ret = vfs_unregister_ramfs_backend();
#endif

  if (ret == ESP_OK) {
    s_is_backend_mounted = false;
  }

  return ret;
}

const char *vfs_get_mount_point(void) {
  return VFS_MOUNT_POINT;
}

const char *vfs_get_backend_name(void) {
  return VFS_BACKEND_NAME;
}

bool vfs_is_mounted_auto(void) {
#ifdef VFS_USE_SD_CARD
  return s_is_backend_mounted && vfs_sdcard_is_mounted();
#endif

#ifdef VFS_USE_SPIFFS
  return s_is_backend_mounted;
#endif

#ifdef VFS_USE_LITTLEFS
  return s_is_backend_mounted && vfs_littlefs_is_mounted();
#endif

#ifdef VFS_USE_RAMFS
  return s_is_backend_mounted;
#endif

  return false;
}

void vfs_print_info(void) {
  ESP_LOGI(TAG, "========== VFS Info ==========");
  ESP_LOGI(TAG, "Backend: %s", VFS_BACKEND_NAME);
  ESP_LOGI(TAG, "Mount Point: %s", VFS_MOUNT_POINT);
  ESP_LOGI(TAG, "Max Files: %d", VFS_MAX_FILES);
  ESP_LOGI(TAG, "Status: %s", vfs_is_mounted_auto() ? "Mounted" : "Unmounted");

  if (vfs_is_mounted_auto()) {
    vfs_statvfs_t stat;
    if (vfs_statvfs(VFS_MOUNT_POINT, &stat) == ESP_OK) {
      ESP_LOGI(TAG, "Total: %" PRIu64 " KB", stat.total_bytes / 1024);
      ESP_LOGI(TAG, "Free: %" PRIu64 " KB", stat.free_bytes / 1024);
      ESP_LOGI(TAG, "Used: %" PRIu64 " KB", stat.used_bytes / 1024);

      float usage =
          stat.total_bytes > 0 ? ((float)stat.used_bytes / stat.total_bytes) * 100.0f : 0.0f;
      ESP_LOGI(TAG, "Usage: %.1f%%", usage);
    }
  }

  ESP_LOGI(TAG, "==============================");
}
