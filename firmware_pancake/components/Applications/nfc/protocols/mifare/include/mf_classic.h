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
 * @file mf_classic.h
 * @brief MIFARE Classic auth, read, write sectors.
 *
 * MIFARE Classic authentication and data access helpers.
 */
#ifndef MF_CLASSIC_H
#define MF_CLASSIC_H

#include <stdint.h>
#include "highboy_nfc_types.h"
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"
#include "crypto1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MIFARE Classic block count constants */
/** @brief Number of blocks in MINI cards (5 sectors × 4 blocks + 1 trailer) */
#define MF_BLOCKS_MINI 20U

/** @brief Number of blocks in 1K cards (16 sectors × 4 blocks) */
#define MF_BLOCKS_1K 64U

/** @brief Number of blocks in 4K cards (32 small + 8 large sectors) */
#define MF_BLOCKS_4K 256U

/* 4K card sector layout constants */
/** @brief First large sector index in 4K cards (32 small sectors, then 8 large) */
#define MF_4K_BIG_SECTOR_START 32U

/** @brief Number of blocks per large sector (16-block sectors) */
#define MF_4K_BIG_BLOCK_COUNT 16U

/** @brief Number of blocks per small sector (4-block sectors) */
#define MF_SMALL_BLOCK_COUNT 4U

/** @brief First block of the large sector area (block 128 = 32 small sectors × 4 blocks) */
#define MF_4K_BIG_BLOCK_BASE 128U

/**
 * @brief Authenticate a sector with key A or B.
 *
 * @param block     Block number within the sector.
 * @param key_type  Key type (A or B).
 * @param key       Pointer to the 6-byte key.
 * @param uid       4-byte card UID.
 * @return
 *   - HB_NFC_OK on success
 *   - Error code on authentication failure
 */
hb_nfc_err_t mf_classic_auth(uint8_t block,
                             mf_key_type_t key_type,
                             const mf_classic_key_t *key,
                             const uint8_t uid[4]);

/**
 * @brief Read a single block (16 bytes).
 *
 * Must be authenticated first.
 *
 * @param block  Absolute block number.
 * @param[out] data  Buffer to receive 16 bytes of block data.
 * @return
 *   - HB_NFC_OK on success
 *   - Error code on failure
 */
hb_nfc_err_t mf_classic_read_block(uint8_t block, uint8_t data[16]);

/**
 * @brief Write a single block (16 bytes).
 *
 * Must be authenticated first.
 *
 * @param block  Absolute block number.
 * @param data   16 bytes to write.
 * @return
 *   - HB_NFC_OK on success
 *   - Error code on failure
 */
hb_nfc_err_t mf_classic_write_block(uint8_t block, const uint8_t data[16]);

/**
 * @brief Write phase (for debugging NACKs).
 */
typedef enum {
  MF_WRITE_PHASE_NONE = 0,
  MF_WRITE_PHASE_CMD,
  MF_WRITE_PHASE_DATA,
} mf_write_phase_t;

/**
 * @brief Get last write phase reached (CMD or DATA).
 *
 * @return Last write phase.
 */
mf_write_phase_t mf_classic_get_last_write_phase(void);

/**
 * @brief Get card type from SAK byte.
 *
 * @param sak  SAK byte from card selection.
 * @return MIFARE Classic card type.
 */
mf_classic_type_t mf_classic_get_type(uint8_t sak);

/**
 * @brief Get number of sectors for a given card type.
 *
 * @param type  MIFARE Classic card type.
 * @return Number of sectors.
 */
int mf_classic_get_sector_count(mf_classic_type_t type);

/**
 * @brief Reset auth state (call before re-select).
 */
void mf_classic_reset_auth(void);

/**
 * @brief Get the last nonce (nt) received from card during auth.
 *
 * Used for PRNG analysis and clone detection.
 *
 * @return Last card nonce value.
 */
uint32_t mf_classic_get_last_nt(void);

/**
 * @brief Copy the current Crypto1 cipher state after a successful auth.
 *
 * Used by the nested attack module to drive its own keystream without
 * touching the static state that mf_classic_read/write_block use.
 *
 * @param[out] out  Destination state struct; must not be NULL.
 */
void mf_classic_get_crypto_state(crypto1_state_t *out);

#ifdef __cplusplus
}
#endif

#endif /* MF_CLASSIC_H */
