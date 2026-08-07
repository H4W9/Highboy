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
 * @file nfc_tag.h
 * @brief NFC Forum Tag Type handlers (Phase 7).
 */
#ifndef NFC_TAG_H
#define NFC_TAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"
#include "highboy_nfc_types.h"

#include "t1t.h"
#include "t4t.h"
#include "felica.h"
#include "iso15693.h"
#include "mf_plus.h"

/**
 * @brief NFC Forum tag types.
 */
typedef enum {
  NFC_TAG_TYPE_UNKNOWN = 0,
  NFC_TAG_TYPE_1 = 1,
  NFC_TAG_TYPE_2,
  NFC_TAG_TYPE_3,
  NFC_TAG_TYPE_4A,
  NFC_TAG_TYPE_4B,
  NFC_TAG_TYPE_6,
  NFC_TAG_TYPE_7,
  NFC_TAG_TYPE_V,
  NFC_TAG_TYPE_COUNT
} nfc_tag_type_t;

/**
 * @brief NFC tag security modes.
 */
typedef enum {
  NFC_TAG_SEC_PLAIN = 0x00,
  NFC_TAG_SEC_MAC = 0x01,
  NFC_TAG_SEC_FULL = 0x03,
} nfc_tag_sec_mode_t;

/**
 * @brief Unified NFC tag descriptor holding protocol-specific data.
 */
typedef struct {
  nfc_tag_type_t type;
  hb_nfc_protocol_t protocol;
  uint8_t uid[NFC_UID_MAX_LEN];
  uint8_t uid_len;
  uint16_t block_size;

  nfc_iso14443a_data_t iso14443a;
  nfc_iso14443b_data_t iso14443b;
  t1t_tag_t t1t;
  felica_tag_t felica;
  iso15693_tag_t iso15693;

  nfc_iso_dep_data_t dep;
  t4t_cc_t t4t_cc;

  uint16_t felica_service;

  mfp_session_t mfp;
  nfc_tag_sec_mode_t sec_mode;
} nfc_tag_t;

/**
 * @brief Get a human-readable string for a tag type.
 *
 * @param type  Tag type enumeration value.
 * @return Pointer to a static string describing the tag type.
 */
const char *nfc_tag_type_str(nfc_tag_type_t type);

/**
 * @brief Detect and identify the tag present on the RF field.
 *
 * @param[out] tag  Tag descriptor to populate with detected tag data.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if tag is NULL
 *   - HB_NFC_ERR_NOT_FOUND if no tag is detected
 */
hb_nfc_err_t nfc_tag_detect(nfc_tag_t *tag);

/**
 * @brief Read a single block from the tag.
 *
 * @param tag        Tag descriptor.
 * @param block_num  Block number to read.
 * @param[out] out       Buffer for the block data.
 * @param out_max    Maximum size of the output buffer.
 * @param[out] out_len   Number of bytes actually read.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if tag or out is NULL
 *   - HB_NFC_ERR on communication failure
 */
hb_nfc_err_t nfc_tag_read_block(
    nfc_tag_t *tag, uint32_t block_num, uint8_t *out, size_t out_max, size_t *out_len);

/**
 * @brief Write a single block to the tag.
 *
 * @param tag        Tag descriptor.
 * @param block_num  Block number to write.
 * @param data       Data to write.
 * @param data_len   Length of the data.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if tag or data is NULL
 *   - HB_NFC_ERR on communication failure
 */
hb_nfc_err_t
nfc_tag_write_block(nfc_tag_t *tag, uint32_t block_num, const uint8_t *data, size_t data_len);

/**
 * @brief Retrieve the UID from a detected tag.
 *
 * @param tag          Tag descriptor.
 * @param[out] uid         Buffer for the UID bytes.
 * @param[out] uid_len     Length of the UID in bytes.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if tag, uid, or uid_len is NULL
 */
hb_nfc_err_t nfc_tag_get_uid(const nfc_tag_t *tag, uint8_t *uid, size_t *uid_len);

/**
 * @brief Perform the first MIFARE Plus authentication for a block.
 *
 * @param tag         Tag descriptor with MFP session state.
 * @param block_addr  Block address to authenticate.
 * @param key         16-byte AES key.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_AUTH on authentication failure
 *   - HB_NFC_ERR on communication failure
 */
hb_nfc_err_t nfc_tag_mfp_auth_first(nfc_tag_t *tag, uint16_t block_addr, const uint8_t key[16]);

/**
 * @brief Perform a subsequent MIFARE Plus authentication (following key switch).
 *
 * @param tag         Tag descriptor with MFP session state.
 * @param block_addr  Block address to authenticate.
 * @param key         16-byte AES key.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_AUTH on authentication failure
 *   - HB_NFC_ERR on communication failure
 */
hb_nfc_err_t nfc_tag_mfp_auth_nonfirst(nfc_tag_t *tag, uint16_t block_addr, const uint8_t key[16]);

/**
 * @brief Set the FeliCa service code used for subsequent read/write operations.
 *
 * @param tag           Tag descriptor.
 * @param service_code  FeliCa service code.
 */
void nfc_tag_set_felica_service(nfc_tag_t *tag, uint16_t service_code);

#ifdef __cplusplus
}
#endif

#endif /* NFC_TAG_H */
