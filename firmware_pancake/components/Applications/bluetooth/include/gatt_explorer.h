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

#ifndef GATT_EXPLORER_H
#define GATT_EXPLORER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Start GATT service and characteristic discovery on a target device.
 *
 * @param addr      Target device BLE address (6 bytes).
 * @param addr_type Target device address type.
 * @return true if exploration started, false on failure.
 */
bool gatt_explorer_start(const uint8_t *addr, uint8_t addr_type);

/**
 * @brief Check if a GATT exploration is currently in progress.
 *
 * @return true if busy, false otherwise.
 */
bool gatt_explorer_is_busy(void);

#ifdef __cplusplus
}
#endif

#endif // GATT_EXPLORER_H
