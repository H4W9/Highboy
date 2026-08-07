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

// Pancake stub: the ESP32-C5 has no SDMMC host peripheral (the P4 target uses
// SDMMC 4-bit). Pancake wires the micro-SD to the shared SPI bus; an SD-over-
// SPI backend is a later phase. For Phase 1 the SD backend reports "no card"
// so the auto VFS falls back to the on-flash LittleFS assets partition.

#include "vfs_sdcard.h"

#include "esp_log.h"

static const char *TAG = "VFS_SDCARD";

esp_err_t vfs_sdcard_init(void) {
  ESP_LOGW(TAG, "SD card unavailable on Pancake (no SDMMC host; SPI SD is TODO)");
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t vfs_sdcard_deinit(void) { return ESP_OK; }

bool vfs_sdcard_is_mounted(void) { return false; }

void vfs_sdcard_print_info(void) {}

esp_err_t vfs_sdcard_format(void) { return ESP_ERR_NOT_SUPPORTED; }

esp_err_t vfs_register_sd_backend(void) { return ESP_ERR_NOT_SUPPORTED; }

esp_err_t vfs_unregister_sd_backend(void) { return ESP_OK; }
