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

#ifndef MT_MOD_ROUTING_H
#define MT_MOD_ROUTING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "mt_modules.h"

/**
 * @brief Initialize the RoutingModule.
 */
void mt_mod_routing_init(uint32_t node_num);

/**
 * @brief Handle an incoming Routing protobuf (ACK/NAK).
 *
 * Currently logs only. ACK/NAK parsing to update retransmit state is
 * deferred to Fase 2 (ReliableRouter).
 */
void mt_mod_routing_on_received(const mt_packet_meta_t *meta, const uint8_t *payload, uint16_t len);

/**
 * @brief Emit an ACK in response to a packet that had want_ack set.
 *
 * Builds Routing { error_reason = NONE } and enqueues it for TX at
 * priority MESH_PRIO_ACK. Target is meta->from, with meta->id carried
 * as request_id.
 */
void mt_mod_routing_maybe_send_ack(const mt_packet_meta_t *meta);

#ifdef __cplusplus
}
#endif

#endif // MT_MOD_ROUTING_H
