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
 * @file mf_key_cache.h
 * @brief Per-card MIFARE Classic key cache with NVS persistence.
 */
#ifndef MF_KEY_CACHE_H
#define MF_KEY_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include "highboy_nfc_types.h"
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MF_KEY_CACHE_MAX_CARDS   16
#define MF_KEY_CACHE_MAX_SECTORS 40

/**
 * @brief One cached card entry: UID and all sector keys found.
 */
typedef struct {
  uint8_t uid[10];
  uint8_t uid_len;
  bool key_a_known[MF_KEY_CACHE_MAX_SECTORS];
  bool key_b_known[MF_KEY_CACHE_MAX_SECTORS];
  uint8_t key_a[MF_KEY_CACHE_MAX_SECTORS][6];
  uint8_t key_b[MF_KEY_CACHE_MAX_SECTORS][6];
  int sector_count;
} mf_key_cache_entry_t;

/**
 * @brief Parameters for mf_key_cache_store().
 */
typedef struct {
  const uint8_t *uid; /**< @brief Card UID. */
  uint8_t uid_len;    /**< @brief UID length in bytes. */
  int sector;         /**< @brief Sector number. */
  int sector_count;   /**< @brief Total sector count for the card. */
  mf_key_type_t type; /**< @brief Key type (A or B). */
  const uint8_t *key; /**< @brief 6-byte key to store. */
} mf_key_cache_store_params_t;

/**
 * @brief Initialize the key cache (load from NVS).
 */
void mf_key_cache_init(void);

/**
 * @brief Look up a cached key for a specific card and sector.
 *
 * @param uid      Card UID.
 * @param uid_len  UID length in bytes.
 * @param sector   Sector number.
 * @param type     Key type (A or B).
 * @param[out] key_out  6-byte key if found.
 * @return true if key was found in cache.
 */
bool mf_key_cache_lookup(
    const uint8_t *uid, uint8_t uid_len, int sector, mf_key_type_t type, uint8_t key_out[6]);

/**
 * @brief Store a key in the cache for a specific card and sector.
 *
 * @param params  Store parameters (uid, sector, key, etc.).
 */
void mf_key_cache_store(const mf_key_cache_store_params_t *params);

/**
 * @brief Persist the cache to NVS.
 */
void mf_key_cache_save(void);

/**
 * @brief Clear all cached keys for a specific card UID.
 *
 * @param uid      Card UID.
 * @param uid_len  UID length in bytes.
 */
void mf_key_cache_clear_uid(const uint8_t *uid, uint8_t uid_len);

/**
 * @brief Get the number of known keys for a specific card.
 *
 * @param uid      Card UID.
 * @param uid_len  UID length in bytes.
 * @return Number of known keys (Key A + Key B).
 */
int mf_key_cache_get_known_count(const uint8_t *uid, uint8_t uid_len);

#ifdef __cplusplus
}
#endif
#endif
