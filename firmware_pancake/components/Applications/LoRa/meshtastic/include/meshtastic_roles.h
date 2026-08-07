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

#ifndef MESHTASTIC_ROLES_H
#define MESHTASTIC_ROLES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Device role identifiers.
 *
 * Values match Config.DeviceConfig.Role in the official Meshtastic
 * protobuf definition (config.proto). Behavior per role is audited
 * against src/mesh/FloodingRouter.cpp and src/mesh/NodeDB.cpp.
 */
typedef enum {
  MT_ROLE_CLIENT = 0,
  MT_ROLE_CLIENT_MUTE = 1,
  MT_ROLE_ROUTER = 2,
  MT_ROLE_ROUTER_CLIENT = 3,
  MT_ROLE_REPEATER = 4,
  MT_ROLE_TRACKER = 5,
  MT_ROLE_SENSOR = 6,
  MT_ROLE_TAK = 7,
  MT_ROLE_CLIENT_HIDDEN = 8,
  MT_ROLE_LOST_AND_FOUND = 9,
  MT_ROLE_TAK_TRACKER = 10,
  MT_ROLE_ROUTER_LATE = 11,
  MT_ROLE_CLIENT_BASE = 12,
  MT_ROLE_COUNT
} mt_role_t;

/**
 * @brief Rebroadcast policy identifiers.
 *
 * Values match RebroadcastMode in config.proto.
 */
typedef enum {
  MT_REBR_ALL = 0,
  MT_REBR_ALL_SKIP_DECODING = 1,
  MT_REBR_LOCAL_ONLY = 2,
  MT_REBR_KNOWN_ONLY = 3,
  MT_REBR_NONE = 4,
  MT_REBR_CORE_PORTNUMS_ONLY = 5,
  MT_REBR_COUNT
} mt_rebr_mode_t;

/**
 * @brief Return the role currently persisted in NVS.
 *
 * Falls back to MT_ROLE_CLIENT if NVS is empty.
 */
mt_role_t mt_role_current(void);

/**
 * @brief Persist the given role and auto-select a matching rebroadcast mode.
 *
 * Side effect: sets the rebroadcast policy based on the role
 * (ROUTER -> CORE_PORTNUMS_ONLY, CLIENT_HIDDEN -> LOCAL_ONLY,
 * REPEATER -> ALL_SKIP_DECODING, others -> ALL).
 */
void mt_role_set(mt_role_t role);

/**
 * @brief Return the printable name for a role.
 */
const char *mt_role_name(mt_role_t role);

/**
 * @brief Return the rebroadcast mode currently persisted in NVS.
 */
mt_rebr_mode_t mt_rebr_mode_current(void);

/**
 * @brief Persist the given rebroadcast mode.
 */
void mt_rebr_mode_set(mt_rebr_mode_t mode);

/**
 * @brief Whether the role relays other nodes' packets at all.
 *
 * CLIENT_MUTE is the only role that always returns false.
 */
bool mt_role_is_rebroadcaster(mt_role_t role);

/**
 * @brief Whether the role cancels its own relay when a duplicate is heard.
 *
 * ROUTER, ROUTER_LATE, and REPEATER never cancel (aggressive infra).
 * All other roles cancel to save airtime.
 */
bool mt_role_cancels_on_duplicate(mt_role_t role);

/**
 * @brief Whether the role uses the late rebroadcast window (longer backoff).
 *
 * Only MT_ROLE_ROUTER_LATE returns true.
 */
bool mt_role_is_late_rebroadcaster(mt_role_t role);

/**
 * @brief Decide whether to forward a packet based on rebroadcast policy.
 *
 * @param mode         Active rebroadcast mode.
 * @param portnum      PortNum of the decoded packet (0 if undecoded).
 * @param is_from_known true if the sender is in the local NodeDB.
 * @return true if this packet should be rebroadcast.
 */
bool mt_rebr_should_forward(mt_rebr_mode_t mode, uint32_t portnum, bool is_from_known);

/**
 * @brief Intervalo de broadcast de NodeInfo em segundos para a role dada.
 *
 * 0 = essa role nao faz broadcast de NodeInfo.
 */
uint32_t mt_role_nodeinfo_interval_secs(mt_role_t role);

/**
 * @brief Intervalo de broadcast de Telemetry em segundos para a role dada.
 *
 * 0 = essa role nao faz broadcast de telemetry.
 */
uint32_t mt_role_telemetry_interval_secs(mt_role_t role);

/**
 * @brief Intervalo de broadcast de Position em segundos para a role dada.
 *
 * 0 = essa role nao faz broadcast periodico de posicao (so on-demand).
 */
uint32_t mt_role_position_interval_secs(mt_role_t role);

/**
 * @brief Se a role marca o no como nao-mensagavel no NodeInfo.
 *
 * ROUTER/ROUTER_LATE/REPEATER: true (infraestrutura, nao chat).
 */
bool mt_role_is_unmessagable(mt_role_t role);

/**
 * @brief Se a role deve responder a requisicoes (want_response).
 *
 * CLIENT_HIDDEN retorna false (invisivel).
 */
bool mt_role_should_respond(mt_role_t role);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_ROLES_H
