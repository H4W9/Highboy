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

#ifndef MT_MOD_POSITION_H
#define MT_MOD_POSITION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "mt_modules.h"

/**
 * @brief Initialize the PositionModule.
 *
 * Loads fixed_position (lat/lon/alt) from NVS if present.
 */
void mt_mod_position_init(uint32_t node_num);

/**
 * @brief Handle an incoming Position protobuf from another node.
 *
 * Replies with our fixed_position if want_response is set and
 * fixed_position is configured.
 */
void mt_mod_position_on_received(const mt_packet_meta_t *meta,
                                 const uint8_t *payload,
                                 uint16_t len);

/**
 * @brief Periodic tick. Broadcasts position every 15 min if fixed.
 */
void mt_mod_position_tick(uint32_t now_s);

/**
 * @brief Set a fixed position (and persist to NVS).
 *
 * @param lat_e7  Latitude in degrees * 1e7.
 * @param lon_e7  Longitude in degrees * 1e7.
 * @param alt_m   Altitude in meters.
 */
void mt_mod_position_set_fixed(int32_t lat_e7, int32_t lon_e7, int32_t alt_m);

/**
 * @brief Remove the fixed position (and delete from NVS).
 */
void mt_mod_position_remove_fixed(void);

#ifdef __cplusplus
}
#endif

#endif // MT_MOD_POSITION_H
