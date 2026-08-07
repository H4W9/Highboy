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

#ifndef CANNED_SPAM_H
#define CANNED_SPAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Descriptor for a BLE spam attack type.
 */
typedef struct {
  const char *name;
} canned_spam_type_t;

/**
 * @brief Get the number of available spam attack types.
 *
 * @return Number of attack types.
 */
int spam_get_attack_count(void);

/**
 * @brief Get the descriptor for a spam attack type by index.
 *
 * @param index Zero-based attack index.
 * @return Pointer to the attack type descriptor, or NULL if index is out of range.
 */
const canned_spam_type_t *spam_get_attack_type(int index);

/**
 * @brief Start a BLE spam attack.
 *
 * @param attack_index Zero-based index of the attack type to start.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_STATE if already running
 *   - ESP_FAIL if bluetooth service is not running
 *   - ESP_ERR_NOT_FOUND if attack_index is invalid
 */
esp_err_t spam_start(int attack_index);

/**
 * @brief Stop the currently running BLE spam attack.
 *
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_STATE if not running
 */
esp_err_t spam_stop(void);

#ifdef __cplusplus
}
#endif

#endif // CANNED_SPAM_H
