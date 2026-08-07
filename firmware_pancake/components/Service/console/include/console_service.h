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

#ifndef CONSOLE_SERVICE_H
#define CONSOLE_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Initialize the console service and register all commands.
 *
 * @return ESP_OK on success.
 */
esp_err_t console_service_init(void);

/**
 * @brief Start the console REPL task.
 */
void console_service_start(void);

/**
 * @brief Register filesystem commands (ls, cd, pwd, cat).
 */
void register_fs_commands(void);

/**
 * @brief Register system commands (free, restart, tasks, ip).
 */
void register_system_commands(void);

/**
 * @brief Register Wi-Fi commands.
 */
void register_wifi_commands(void);

#ifdef __cplusplus
}
#endif

#endif // CONSOLE_SERVICE_H
