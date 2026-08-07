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

#ifndef MESHTASTIC_PKT_HISTORY_H
#define MESHTASTIC_PKT_HISTORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define MT_PKT_HISTORY_MAX 128
#define MT_NUM_RELAYERS    6

/**
 * @brief Initialize the ring buffer. Idempotent.
 */
void mt_pkt_history_init(void);

/**
 * @brief Check whether a packet has been seen and update the registry.
 *
 * Implements the dedup + relayer tracking + hop upgrade detection logic
 * described in src/mesh/PacketHistory.cpp of the official firmware.
 *
 * @param sender             Originator node number.
 * @param id                 Packet id.
 * @param hop_limit          Current hop_limit seen in this reception.
 * @param relayer_id         Last byte of relay_node (0 if unknown).
 * @param[out] out_upgraded  Set to true when hop_limit exceeds the highest
 *                           previously seen value. May be NULL.
 * @return true if already seen (possibly with lower hop_limit - pure dedup).
 */
bool mt_pkt_history_check_add(
    uint32_t sender, uint32_t id, uint8_t hop_limit, uint8_t relayer_id, bool *out_upgraded);

/**
 * @brief Check whether a specific node has already relayed a packet.
 *
 * @param relayer_id  Last byte of relay_node to query.
 * @param sender      Originator node number.
 * @param id          Packet id.
 * @return true if that relayer appears in the record's relayers list.
 */
bool mt_pkt_history_was_relayer(uint8_t relayer_id, uint32_t sender, uint32_t id);

/**
 * @brief Remove a relayer from the list for a specific packet.
 *
 * Used when recomputing next-hop hints after a route change.
 */
void mt_pkt_history_remove_relayer(uint8_t relayer_id, uint32_t sender, uint32_t id);

/**
 * @brief Count of active (non-evicted) entries. Debug helper.
 */
int mt_pkt_history_count(void);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_PKT_HISTORY_H
