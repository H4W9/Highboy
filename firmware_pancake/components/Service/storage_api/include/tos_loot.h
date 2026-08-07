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
 * @file tos_loot.h
 * @brief Unique loot filename generator with date prefix.
 */

#ifndef TOS_LOOT_H
#define TOS_LOOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief Generate a unique loot file path with date prefix.
 *
 * Uses YYYY-MM-DD if RTC/NTP is set, otherwise uptime-based.
 * Example: "2026-03-29_handshake_01.pcap" or "boot0142_handshake_01.pcap"
 *
 * Increments the index until a non-existing filename is found (max 99).
 *
 * @param dir        Target directory (e.g. TOS_PATH_WIFI_LOOT_HS).
 * @param prefix     File prefix (e.g. "handshake").
 * @param ext        Extension without dot (e.g. "pcap").
 * @param out        Output buffer for the full path.
 * @param out_size   Size of the out buffer.
 * @param out_name   Output buffer for filename only (can be NULL).
 * @param name_size  Size of the out_name buffer.
 */
void tos_loot_generate_path(const char *dir,
                            const char *prefix,
                            const char *ext,
                            char *out,
                            size_t out_size,
                            char *out_name,
                            size_t name_size);

#ifdef __cplusplus
}
#endif

#endif // TOS_LOOT_H
