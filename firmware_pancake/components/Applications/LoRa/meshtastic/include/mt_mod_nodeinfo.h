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

#ifndef MT_MOD_NODEINFO_H
#define MT_MOD_NODEINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "mt_modules.h"

/**
 * @brief Initialize the NodeInfoModule.
 *
 * Loads the persisted long_name and short_name from NVS. Falls back
 * to "Highboy" and "HBX" on first boot.
 */
void mt_mod_nodeinfo_init(uint32_t node_num);

/**
 * @brief Handle an incoming User protobuf from another node.
 *
 * If meta->want_response is set, replies with our own User (subject to
 * a 12h throttle to avoid broadcast storms).
 */
void mt_mod_nodeinfo_on_received(const mt_packet_meta_t *meta,
                                 const uint8_t *payload,
                                 uint16_t len);

/**
 * @brief Periodic tick (call once per second).
 *
 * Triggers a NodeInfo broadcast every 15 min (default interval).
 */
void mt_mod_nodeinfo_tick(uint32_t now_s);

/**
 * @brief Set our display names and persist them to NVS.
 *
 * @param long_name   Null-terminated string, up to 31 chars. NULL to keep.
 * @param short_name  Null-terminated string, up to 7 chars. NULL to keep.
 *
 * Forces the next periodic broadcast to fire immediately.
 */
void mt_mod_nodeinfo_set_name(const char *long_name, const char *short_name);

/**
 * @brief Return the current long name. Pointer remains valid until next set.
 */
const char *mt_mod_nodeinfo_get_long(void);

/**
 * @brief Return the current short name.
 */
const char *mt_mod_nodeinfo_get_short(void);

/**
 * @brief Send a unicast NodeInfo request to an unknown peer.
 *
 * Transmits our own User proto to `to` with want_response=true, asking
 * the peer to reply with its NodeInfo. Internally rate-limited per peer
 * so repeated calls for the same `to` within the throttle window are
 * dropped silently. This is the official discovery mechanism used when
 * a packet arrives from a node not yet present in the NodeDB.
 */
void mt_mod_nodeinfo_request(uint32_t to);

#ifdef __cplusplus
}
#endif

#endif // MT_MOD_NODEINFO_H
