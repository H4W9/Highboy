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

#ifndef MESHTASTIC_CHANNELS_H
#define MESHTASTIC_CHANNELS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define MT_MAX_CHANNELS 8
#define MT_PSK_SIZE     16

/**
 * @brief Channel role.
 *
 * Matches Channel.Role in the official protobuf.
 */
typedef enum {
  MT_CH_DISABLED = 0,
  MT_CH_PRIMARY = 1,
  MT_CH_SECONDARY = 2,
  MT_CH_COUNT
} mt_channel_role_t;

/**
 * @brief Persisted state of a single channel slot.
 */
typedef struct {
  bool is_used;
  mt_channel_role_t role;
  uint8_t psk[MT_PSK_SIZE];
  uint8_t hash;
  char name[32];
  uint32_t id;
} mt_channel_t;

/**
 * @brief Load channels from NVS or install the default PRIMARY on first boot.
 */
void mt_channels_init(void);

/**
 * @brief Read a channel slot by index.
 *
 * @param idx  Slot index in [0, MT_MAX_CHANNELS).
 * @return Pointer to the internal slot, or NULL if idx is out of range.
 */
const mt_channel_t *mt_channel_get(uint8_t idx);

/**
 * @brief Configure a channel slot and persist to NVS.
 *
 * Computes the 8-bit hash as SHA256(psk)[0]. If role is MT_CH_PRIMARY,
 * any other slot currently marked PRIMARY is demoted to SECONDARY.
 *
 * @param idx   Slot index in [0, MT_MAX_CHANNELS).
 * @param name  Optional display name. May be NULL to keep the existing name.
 * @param psk   Optional 16-byte PSK. May be NULL to keep the existing PSK.
 * @param role  New role. MT_CH_DISABLED clears is_used.
 */
void mt_channel_set(uint8_t idx, const char *name, const uint8_t *psk, mt_channel_role_t role);

/**
 * @brief Set the user-assigned channel id (fixed32 from app) and persist.
 */
void mt_channel_set_id(uint8_t idx, uint32_t id);

/**
 * @brief Disable a channel slot and persist to NVS.
 */
void mt_channel_disable(uint8_t idx);

/**
 * @brief Find an enabled channel whose hash matches.
 *
 * @param hash         8-bit channel hash to match.
 * @param[out] out_idx  Slot index on hit. May be NULL.
 * @param[out] out_psk  PSK pointer on hit. May be NULL.
 * @return true if a matching enabled channel was found.
 */
bool mt_channel_lookup_by_hash(uint8_t hash, uint8_t *out_idx, const uint8_t **out_psk);

/**
 * @brief Return the hash of the primary channel.
 */
uint8_t mt_channel_primary_hash(void);

/**
 * @brief Return the PSK of the primary channel.
 */
const uint8_t *mt_channel_primary_psk(void);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_CHANNELS_H
