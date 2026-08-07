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

#ifndef SYS_MONITOR_H
#define SYS_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Start the system monitor task.
 *
 * Launches a background task pinned to Core 1 that periodically
 * checks all running tasks for critical stack usage. If a task's
 * stack high watermark drops below the threshold, the monitor
 * terminates it and displays a safeguard alert.
 *
 * When verbose logging is enabled, RAM usage (internal + PSRAM)
 * is printed every monitoring cycle.
 *
 * @param is_verbose  Enable periodic RAM usage logging.
 */
void sys_monitor_start(bool is_verbose);

#ifdef __cplusplus
}
#endif

#endif // SYS_MONITOR_H
