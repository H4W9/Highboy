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
 * @file mf_classic_writer.c
 * @brief MIFARE Classic write and access-bit helpers.
 * Reference: NXP MF1S50YYX_V1 — MIFARE Classic 1K/4K datasheet, §8 (access conditions).
 */
#include "mf_classic_writer.h"

#include <string.h>

#include "esp_log.h"

#include "poller.h"
#include "mf_classic.h"
#include "nfc_poller.h"

static const char *TAG = "mf_classic_writer";

#define MF_ACCESS_GROUPS 4

#define MF_KEY_LEN    6
#define MF_BLOCK_SIZE 16

#define MF_BLOCK_MANUFACTURER 0

#define MF_TRAILER_BYTE_AC0   6
#define MF_TRAILER_BYTE_AC1   7
#define MF_TRAILER_BYTE_AC2   8
#define MF_TRAILER_GPB_OFFSET 9

#define MF_ACK_NIBBLE_MASK 0x0F
#define MF_ACK_VALUE       0x0A

const uint8_t MF_ACCESS_BITS_DEFAULT[3] = {0xFF, 0x07, 0x80};
const uint8_t MF_ACCESS_BITS_READ_ONLY[3] = {0x78, 0x77, 0x88};

const char *mf_write_result_str(mf_write_result_t r) {
  switch (r) {
    case MF_WRITE_OK:
      return "OK";
    case MF_WRITE_ERR_RESELECT:
      return "reselect failed";
    case MF_WRITE_ERR_AUTH:
      return "authentication denied";
    case MF_WRITE_ERR_CMD_NACK:
      return "WRITE command NACK";
    case MF_WRITE_ERR_DATA_NACK:
      return "data NACK";
    case MF_WRITE_ERR_VERIFY:
      return "verification failed";
    case MF_WRITE_ERR_PROTECTED:
      return "block protected";
    case MF_WRITE_ERR_PARAM:
      return "invalid parameter";
    default:
      return "unknown error";
  }
}

static bool access_bit_is_valid(uint8_t v) {
  return (v == 0U || v == 1U);
}

bool mf_classic_access_bits_encode(const mf_classic_access_bits_t *ac, uint8_t out_access_bits[3]) {
  if (ac == NULL || out_access_bits == NULL)
    return false;

  uint8_t b6 = 0;
  uint8_t b7 = 0;
  uint8_t b8 = 0;

  for (uint8_t grp = 0; grp < MF_ACCESS_GROUPS; grp++) {
    uint8_t c1 = ac->c1[grp];
    uint8_t c2 = ac->c2[grp];
    uint8_t c3 = ac->c3[grp];

    if (!access_bit_is_valid(c1) || !access_bit_is_valid(c2) || !access_bit_is_valid(c3))
      return false;

    if (c1)
      b7 |= (uint8_t)(1U << (4 + grp));
    else
      b6 |= (uint8_t)(1U << grp);

    if (c2)
      b8 |= (uint8_t)(1U << grp);
    else
      b6 |= (uint8_t)(1U << (4 + grp));

    if (c3)
      b8 |= (uint8_t)(1U << (4 + grp));
    else
      b7 |= (uint8_t)(1U << grp);
  }

  out_access_bits[0] = b6;
  out_access_bits[1] = b7;
  out_access_bits[2] = b8;
  return true;
}

bool mf_classic_access_bits_valid(const uint8_t access_bits[3]) {
  if (access_bits == NULL)
    return false;

  uint8_t b6 = access_bits[0];
  uint8_t b7 = access_bits[1];
  uint8_t b8 = access_bits[2];

  for (uint8_t grp = 0; grp < MF_ACCESS_GROUPS; grp++) {
    uint8_t c1 = (b7 >> (4 + grp)) & 1U;
    uint8_t c1_inv = (uint8_t)((~b6 >> grp) & 1U);
    uint8_t c2 = (b8 >> grp) & 1U;
    uint8_t c2_inv = (uint8_t)((~b6 >> (4 + grp)) & 1U);
    uint8_t c3 = (b8 >> (4 + grp)) & 1U;
    uint8_t c3_inv = (uint8_t)((~b7 >> grp) & 1U);
    if (c1 != c1_inv || c2 != c2_inv || c3 != c3_inv)
      return false;
  }

  return true;
}

static int total_blocks(mf_classic_type_t type) {
  switch (type) {
    case MF_CLASSIC_MINI:
      return MF_BLOCKS_MINI;
    case MF_CLASSIC_1K:
      return MF_BLOCKS_1K;
    case MF_CLASSIC_4K:
      return MF_BLOCKS_4K;
    default:
      return MF_BLOCKS_1K;
  }
}

static int sector_block_count(mf_classic_type_t type, int sector) {
  if (type == MF_CLASSIC_4K && sector >= MF_4K_BIG_SECTOR_START)
    return MF_4K_BIG_BLOCK_COUNT;
  return MF_SMALL_BLOCK_COUNT;
}

static int sector_first_block(mf_classic_type_t type, int sector) {
  if (type == MF_CLASSIC_4K && sector >= MF_4K_BIG_SECTOR_START)
    return MF_4K_BIG_BLOCK_BASE + (sector - MF_4K_BIG_SECTOR_START) * MF_4K_BIG_BLOCK_COUNT;
  return sector * MF_SMALL_BLOCK_COUNT;
}

static int sector_trailer_block(mf_classic_type_t type, int sector) {
  return sector_first_block(type, sector) + sector_block_count(type, sector) - 1;
}

static int block_to_sector(mf_classic_type_t type, int block) {
  if (type == MF_CLASSIC_4K && block >= MF_4K_BIG_BLOCK_BASE)
    return MF_4K_BIG_SECTOR_START + (block - MF_4K_BIG_BLOCK_BASE) / MF_4K_BIG_BLOCK_COUNT;
  return block / MF_SMALL_BLOCK_COUNT;
}

static bool is_trailer_block(mf_classic_type_t type, int block) {
  int sector = block_to_sector(type, block);
  return block == sector_trailer_block(type, sector);
}

mf_write_result_t mf_classic_write_block_raw(uint8_t block, const uint8_t data[16]) {
  hb_nfc_err_t err = mf_classic_write_block(block, data);
  if (err == HB_NFC_OK)
    return MF_WRITE_OK;
  if (err == HB_NFC_ERR_AUTH)
    return MF_WRITE_ERR_AUTH;
  if (err == HB_NFC_ERR_NACK) {
    mf_write_phase_t phase = mf_classic_get_last_write_phase();
    return (phase == MF_WRITE_PHASE_DATA) ? MF_WRITE_ERR_DATA_NACK : MF_WRITE_ERR_CMD_NACK;
  }
  return MF_WRITE_ERR_CMD_NACK;
}

mf_write_result_t mf_classic_write(nfc_iso14443a_data_t *card,
                                   uint8_t block,
                                   const uint8_t data[16],
                                   const uint8_t key[6],
                                   mf_key_type_t key_type,
                                   bool verify,
                                   bool allow_special) {
  if (card == NULL || data == NULL || key == NULL)
    return MF_WRITE_ERR_PARAM;

  mf_classic_type_t type = mf_classic_get_type(card->sak);
  if ((int)block >= total_blocks(type))
    return MF_WRITE_ERR_PARAM;

  if (block == MF_BLOCK_MANUFACTURER && !allow_special) {
    ESP_LOGE(TAG, "Block 0 (manufacturer) protected use allow_special=true only on magic cards");
    return MF_WRITE_ERR_PROTECTED;
  }
  if (is_trailer_block(type, block) && !allow_special) {
    ESP_LOGE(TAG, "Block %d is a trailer use allow_special=true and verify access bits!", block);
    return MF_WRITE_ERR_PROTECTED;
  }

  mf_classic_reset_auth();
  hb_nfc_err_t err = iso14443a_poller_reselect(card);
  if (err != HB_NFC_OK) {
    ESP_LOGE(TAG, "Reselect failed: %d", err);
    return MF_WRITE_ERR_RESELECT;
  }

  mf_classic_key_t k;
  memcpy(k.data, key, MF_KEY_LEN);

  err = mf_classic_auth(block, key_type, &k, card->uid);
  if (err != HB_NFC_OK) {
    ESP_LOGE(TAG, "Auth failed on block %d (key%c)", block, key_type == MF_KEY_A ? 'A' : 'B');
    return MF_WRITE_ERR_AUTH;
  }

  mf_write_result_t wres = mf_classic_write_block_raw(block, data);
  if (wres != MF_WRITE_OK) {
    ESP_LOGE(TAG, "Write failed (block %d): %s", block, mf_write_result_str(wres));
    return wres;
  }

  ESP_LOGI(TAG, "Block %d written", block);

  if (verify) {
    uint8_t readback[MF_BLOCK_SIZE] = {0};
    err = mf_classic_read_block(block, readback);
    if (err != HB_NFC_OK) {
      ESP_LOGW(TAG, "Verify: read failed (block %d)", block);
      return MF_WRITE_ERR_VERIFY;
    }
    if (memcmp(data, readback, MF_BLOCK_SIZE) != 0) {
      ESP_LOGE(TAG, "Verify: read data mismatch (block %d)!", block);
      ESP_LOG_BUFFER_HEX("expected", data, MF_BLOCK_SIZE);
      ESP_LOG_BUFFER_HEX("readback", readback, MF_BLOCK_SIZE);
      return MF_WRITE_ERR_VERIFY;
    }
    ESP_LOGI(TAG, "Block %d verified", block);
  }

  return MF_WRITE_OK;
}

int mf_classic_write_sector(nfc_iso14443a_data_t *card,
                            uint8_t sector,
                            const uint8_t *data,
                            const uint8_t key[6],
                            mf_key_type_t key_type,
                            bool verify) {
  if (card == NULL || data == NULL || key == NULL)
    return -1;

  mf_classic_type_t type = mf_classic_get_type(card->sak);
  int sector_count = mf_classic_get_sector_count(type);
  if ((int)sector >= sector_count)
    return -1;

  const int blocks_in_sector = sector_block_count(type, sector);
  const int data_blocks = blocks_in_sector - 1;
  const int fb = sector_first_block(type, sector);
  const int last_data_block = fb + data_blocks - 1;

  ESP_LOGI(TAG, "Writing sector %d (blocks %d..%d)...", sector, fb, last_data_block);

  mf_classic_reset_auth();
  hb_nfc_err_t err = iso14443a_poller_reselect(card);
  if (err != HB_NFC_OK) {
    ESP_LOGE(TAG, "Reselect failed for sector %d", sector);
    return -1;
  }

  mf_classic_key_t k;
  memcpy(k.data, key, MF_KEY_LEN);

  err = mf_classic_auth(fb, key_type, &k, card->uid);
  if (err != HB_NFC_OK) {
    ESP_LOGE(TAG, "Auth failed on sector %d (key%c)", sector, key_type == MF_KEY_A ? 'A' : 'B');
    return -1;
  }

  int written = 0;
  for (int b = 0; b < data_blocks; b++) {
    uint8_t block = (uint8_t)(fb + b);
    const uint8_t *block_data = data + (b * MF_BLOCK_SIZE);

    mf_write_result_t wres = mf_classic_write_block_raw(block, block_data);
    if (wres != MF_WRITE_OK) {
      ESP_LOGE(TAG, "Write failed on block %d: %s", block, mf_write_result_str(wres));
      break;
    }

    if (verify) {
      mf_classic_reset_auth();
      err = iso14443a_poller_reselect(card);
      if (err != HB_NFC_OK) {
        ESP_LOGE(TAG, "Reselect failed during verify (block %d)", block);
        return written;
      }
      err = mf_classic_auth(fb, key_type, &k, card->uid);
      if (err != HB_NFC_OK) {
        ESP_LOGE(TAG,
                 "Auth failed during verify (sector %d, key%c)",
                 sector,
                 key_type == MF_KEY_A ? 'A' : 'B');
        return written;
      }

      uint8_t readback[MF_BLOCK_SIZE] = {0};
      err = mf_classic_read_block(block, readback);
      if (err != HB_NFC_OK || memcmp(block_data, readback, MF_BLOCK_SIZE) != 0) {
        ESP_LOGE(TAG, "Verify failed on block %d!", block);
        return written;
      }
      ESP_LOGI(TAG, "Block %d written and verified", block);

      if (b < data_blocks - 1) {
        mf_classic_reset_auth();
        err = iso14443a_poller_reselect(card);
        if (err != HB_NFC_OK) {
          ESP_LOGE(TAG, "Reselect failed to continue (sector %d)", sector);
          return written;
        }
        err = mf_classic_auth(fb, key_type, &k, card->uid);
        if (err != HB_NFC_OK) {
          ESP_LOGE(TAG,
                   "Auth failed to continue (sector %d, key%c)",
                   sector,
                   key_type == MF_KEY_A ? 'A' : 'B');
          return written;
        }
      }
    } else {
      ESP_LOGI(TAG, "Block %d written", block);
    }

    written++;
  }

  ESP_LOGI(TAG, "Sector %d: %d/%d blocks written", sector, written, data_blocks);
  return written;
}

void mf_classic_build_trailer(const uint8_t key_a[6],
                              const uint8_t key_b[6],
                              const uint8_t access_bits[3],
                              uint8_t out_trailer[16]) {
  memcpy(out_trailer, key_a, MF_KEY_LEN);

  const uint8_t *ac = (access_bits != NULL) ? access_bits : MF_ACCESS_BITS_DEFAULT;
  out_trailer[MF_TRAILER_BYTE_AC0] = ac[0];
  out_trailer[MF_TRAILER_BYTE_AC1] = ac[1];
  out_trailer[MF_TRAILER_BYTE_AC2] = ac[2];

  out_trailer[MF_TRAILER_GPB_OFFSET] = 0x00;

  memcpy(&out_trailer[MF_TRAILER_GPB_OFFSET + 1], key_b, MF_KEY_LEN);
}

bool mf_classic_build_trailer_safe(const uint8_t key_a[6],
                                   const uint8_t key_b[6],
                                   const mf_classic_access_bits_t *ac,
                                   uint8_t gpb,
                                   uint8_t out_trailer[16]) {
  if (key_a == NULL || key_b == NULL || ac == NULL || out_trailer == NULL)
    return false;

  uint8_t access_bits[3];
  if (!mf_classic_access_bits_encode(ac, access_bits))
    return false;
  if (!mf_classic_access_bits_valid(access_bits))
    return false;

  memcpy(out_trailer, key_a, MF_KEY_LEN);
  out_trailer[MF_TRAILER_BYTE_AC0] = access_bits[0];
  out_trailer[MF_TRAILER_BYTE_AC1] = access_bits[1];
  out_trailer[MF_TRAILER_BYTE_AC2] = access_bits[2];
  out_trailer[MF_TRAILER_GPB_OFFSET] = gpb;
  memcpy(&out_trailer[MF_TRAILER_GPB_OFFSET + 1], key_b, MF_KEY_LEN);
  return true;
}
