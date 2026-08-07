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

#ifndef MT_MOD_TEXT_H
#define MT_MOD_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "mt_modules.h"

/**
 * @brief Initialize the TextMessageModule.
 *
 * Clears the 50-slot dedup ring and records our node number.
 */
void mt_mod_text_init(uint32_t node_num);

/**
 * @brief Handle an incoming text message.
 *
 * Deduplicates on packet id (ring buffer of the last 50 ids).
 * Logs the message. Forwarding to the app happens in the mesh core via
 * PhoneAPI; this handler does not push frames directly.
 *
 * @param meta        Packet metadata.
 * @param payload     UTF-8 bytes (or Unishox2 if compressed).
 * @param len         Payload length.
 * @param is_compressed True if the packet arrived on portnum 7 (compressed).
 */
void mt_mod_text_on_received(const mt_packet_meta_t *meta,
                             const uint8_t *payload,
                             uint16_t len,
                             bool is_compressed);

#ifdef __cplusplus
}
#endif

#endif // MT_MOD_TEXT_H
