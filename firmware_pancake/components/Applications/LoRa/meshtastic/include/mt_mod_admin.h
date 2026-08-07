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

#ifndef MT_MOD_ADMIN_H
#define MT_MOD_ADMIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "mt_modules.h"

/**
 * @brief Initialize the AdminModule.
 *
 * Clears the session passkey. No packet is sent during init.
 */
void mt_mod_admin_init(uint32_t node_num);

/**
 * @brief Handle an incoming AdminMessage.
 *
 * Parses the oneof payload_variant and dispatches to the matching
 * handler. Remote SET operations require a valid session passkey
 * (issued by a prior GET within the last 300s).
 *
 * Implemented variants (Fase 1.2 subset):
 *   - get_owner_request, set_owner
 *   - get_config_request, set_config (partial)
 *   - get_device_metadata_request
 *   - reboot_seconds, shutdown_seconds
 *   - factory_reset_config, factory_reset_device, nodedb_reset
 *   - set_fixed_position, remove_fixed_position
 *   - begin_edit_settings, commit_edit_settings
 */
void mt_mod_admin_on_received(const mt_packet_meta_t *meta, const uint8_t *payload, uint16_t len);

/**
 * @brief Periodic tick. Triggers deferred reboot if scheduled.
 */
void mt_mod_admin_tick(void);

#ifdef __cplusplus
}
#endif

#endif // MT_MOD_ADMIN_H
