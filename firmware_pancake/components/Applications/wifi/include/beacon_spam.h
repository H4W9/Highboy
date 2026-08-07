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

#ifndef BEACON_SPAM_H
#define BEACON_SPAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Start beacon spam using a custom SSID list from a JSON file.
 *
 * @param json_path  Path to the beacon list JSON file.
 * @return true on success, false on failure.
 */
bool beacon_spam_start_custom(const char *json_path);

/**
 * @brief Start beacon spam with randomly generated SSIDs.
 *
 * @return true on success, false on failure.
 */
bool beacon_spam_start_random(void);

/**
 * @brief Stop the beacon spam attack.
 */
void beacon_spam_stop(void);

/**
 * @brief Check if beacon spam is currently running.
 *
 * @return true if running, false otherwise.
 */
bool beacon_spam_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // BEACON_SPAM_H
