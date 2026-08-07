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

#ifndef TOS_FIRST_BOOT_H
#define TOS_FIRST_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Creates the full SD card folder structure on first boot.
 *
 * Checks for a marker file (.setup_done) to avoid re-running.
 * If the marker is absent, creates all protocol folders, sub-folders,
 * system directories, and the config directory. Never overwrites
 * existing data — only creates what is missing.
 *
 * @return ESP_OK on success or if setup was already done
 */
esp_err_t tos_first_boot_setup(void);

#ifdef __cplusplus
}
#endif

#endif // TOS_FIRST_BOOT_H
