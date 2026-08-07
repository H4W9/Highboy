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

// Pancake stub: no SDMMC host on the ESP32-C5. See vfs_sdcard.c. All SD
// operations report "not mounted" so the UI degrades gracefully.

#include "sd_card_init.h"

#include "esp_log.h"

static const char *TAG = "SD_CARD";

esp_err_t sd_init(void) {
  ESP_LOGW(TAG, "SD card unavailable on Pancake (no SDMMC host)");
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sd_init_custom(uint8_t max_files, bool format_if_failed) {
  (void)max_files;
  (void)format_if_failed;
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sd_deinit(void) { return ESP_OK; }

bool sd_is_mounted(void) { return false; }

esp_err_t sd_remount(void) { return ESP_ERR_NOT_SUPPORTED; }

esp_err_t sd_check_health(void) { return ESP_ERR_NOT_SUPPORTED; }
