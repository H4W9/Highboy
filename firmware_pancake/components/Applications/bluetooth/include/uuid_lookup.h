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

#ifndef UUID_LOOKUP_H
#define UUID_LOOKUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Look up a human-readable name for a BLE UUID16 value.
 *
 * @param uuid16 The 16-bit UUID to look up.
 * @return Name string, or "Unknown Service/Char" if not found.
 */
const char *uuid_get_name_by_u16(uint16_t uuid16);

#ifdef __cplusplus
}
#endif

#endif // UUID_LOOKUP_H
