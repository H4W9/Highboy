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
 * @file iso15693_emu.c
 * @brief ISO 15693 (NFC-V) tag emulation over ST25R3916 passive target mode.
 */
#include "iso15693_emu.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "st25r3916_core.h"
#include "st25r3916_reg.h"
#include "st25r3916_cmd.h"
#include "st25r3916_fifo.h"
#include "st25r3916_irq.h"
#include "hb_nfc_spi.h"

static const char *TAG = "ISO15693_EMU";

#define MODE_TARGET_NFCV    0xB0U
#define OP_CTRL_TARGET      0xC3U
#define TIMER_I_EON         0x10U
#define ACTIVE_IDLE_TIMEOUT 2000U
#define RESP_FLAGS_OK       0x00U
#define RESP_FLAGS_ERR      0x01U

#define AUX_OSC_BIT                  0x04U
#define FIFO_LEN_MASK                0x7FU
#define PTA_STATE_MASK               0x0FU
#define ISO15693_SYSINFO_ALL_FLAGS   0x0FU
#define ISO15693_BLOCK_SIZE_MASK     0x1FU
#define ISO15693_ADDRESSED_DATA_OFF  10
#define ISO15693_DATA_OFF            2
#define ISO15693_IO_CONF2_SUP_AAT    0x80U
#define ISO15693_PT_MOD_OOK          0x60U
#define ISO15693_OSC_TIMEOUT_MS      200
#define ISO15693_SENSE_IDLE_MAX      250U
#define ISO15693_ACTIVE_LOG_INTERVAL 500U
#define ISO15693_EMU_DEFAULT_BLOCKS  8
#define ISO15693_EMU_DEFAULT_BSIZE   4
#define ISO15693_DELAY_SHORT_MS      2
#define ISO15693_DELAY_MEDIUM_MS     5
#define ISO15693_DELAY_LONG_MS       10
#define ISO15693_DELAY_1MS           1

typedef enum {
  ISO15693_STATE_SLEEP,
  ISO15693_STATE_SENSE,
  ISO15693_STATE_ACTIVE,
} iso15693_emu_state_t;

static bool wait_oscillator(int timeout_ms);
static void tx_response(const uint8_t *data, int len);
static void tx_error(uint8_t err_code);
static int fifo_rx(uint8_t *buf, size_t max);
static bool uid_matches(const uint8_t *frame, int len, int uid_offset);
static bool is_for_us(const uint8_t *frame, int len);
static void handle_inventory(const uint8_t *frame, int len);
static void handle_get_system_info(const uint8_t *frame, int len);
static void handle_read_single_block(const uint8_t *frame, int len);
static void handle_write_single_block(const uint8_t *frame, int len);
static void handle_read_multiple_blocks(const uint8_t *frame, int len);
static void handle_lock_block(const uint8_t *frame, int len);
static void handle_write_afi(const uint8_t *frame, int len);
static void handle_lock_afi(const uint8_t *frame, int len);
static void handle_write_dsfid(const uint8_t *frame, int len);
static void handle_lock_dsfid(const uint8_t *frame, int len);
static void handle_get_multi_block_sec(const uint8_t *frame, int len);
static void handle_stay_quiet(const uint8_t *frame, int len);

static iso15693_emu_card_t s_card;
static iso15693_emu_state_t s_state = ISO15693_STATE_SLEEP;
static bool s_quiet = false;
static bool s_init_done = false;
static uint32_t s_sense_idle = 0;
static uint32_t s_active_idle = 0;
static bool s_block_locked[ISO15693_EMU_MAX_BLOCKS];
static bool s_afi_locked = false;
static bool s_dsfid_locked = false;

static bool wait_oscillator(int timeout_ms) {
  for (int i = 0; i < timeout_ms; i++) {
    uint8_t aux = 0, mi = 0;
    hb_nfc_spi_reg_read(ST25R3916_REG_AUX_DISPLAY, &aux);
    hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &mi);
    if ((aux & AUX_OSC_BIT) || (mi & ST25R3916_IRQ_MAIN_OSC)) {
      ESP_LOGI(TAG, "Osc OK in %dms", i);
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_1MS));
  }
  ESP_LOGW(TAG, "Osc timeout - continuing");
  return false;
}

static void tx_response(const uint8_t *data, int len) {
  if (data == NULL || len <= 0)
    return;
  st25r3916_fifo_clear();
  st25r3916_fifo_set_tx_bytes((uint16_t)len, 0);
  st25r3916_fifo_load(data, (size_t)len);
  hb_nfc_spi_direct_cmd(ST25R3916_CMD_TX_WITH_CRC);
}

static void tx_error(uint8_t err_code) {
  uint8_t resp[2] = {RESP_FLAGS_ERR, err_code};
  tx_response(resp, 2);
}

static int fifo_rx(uint8_t *buf, size_t max) {
  uint8_t fs1 = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_FIFO_STATUS1, &fs1);
  int n = (int)(fs1 & FIFO_LEN_MASK);
  if (n <= 0)
    return 0;
  if ((size_t)n > max)
    n = (int)max;
  st25r3916_fifo_read(buf, (size_t)n);
  return n;
}

static bool uid_matches(const uint8_t *frame, int len, int uid_offset) {
  if (len < uid_offset + (int)ISO15693_UID_LEN)
    return false;
  return (memcmp(&frame[uid_offset], s_card.uid, ISO15693_UID_LEN) == 0);
}

static bool is_for_us(const uint8_t *frame, int len) {
  if (len < 2)
    return false;
  uint8_t flags = frame[0];
  if (flags & ISO15693_FLAG_SELECT)
    return (s_state == ISO15693_STATE_ACTIVE);
  if (flags & ISO15693_FLAG_ADDRESS)
    return uid_matches(frame, len, ISO15693_DATA_OFF);
  return true;
}

static void handle_inventory(const uint8_t *frame, int len) {
  (void)len;
  if (s_quiet)
    return; /* STAY_QUIET: do not respond to INVENTORY */

  uint8_t resp[10];
  resp[0] = RESP_FLAGS_OK;
  resp[1] = s_card.dsfid;
  memcpy(&resp[2], s_card.uid, ISO15693_UID_LEN);

  ESP_LOGI(TAG, "INVENTORY -> replying with UID");
  tx_response(resp, sizeof(resp));

  s_state = ISO15693_STATE_ACTIVE;
  s_quiet = false;
}

static void handle_get_system_info(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;

  uint8_t resp[32];
  int pos = 0;

  resp[pos++] = RESP_FLAGS_OK;
  resp[pos++] = ISO15693_SYSINFO_ALL_FLAGS;
  memcpy(&resp[pos], s_card.uid, ISO15693_UID_LEN);
  pos += ISO15693_UID_LEN;
  resp[pos++] = s_card.dsfid;
  resp[pos++] = s_card.afi;
  resp[pos++] = (uint8_t)(s_card.block_count - 1U);
  resp[pos++] = (uint8_t)((s_card.block_size - 1U) & ISO15693_BLOCK_SIZE_MASK);
  resp[pos++] = s_card.ic_ref;

  ESP_LOGI(TAG, "GET_SYSTEM_INFO -> blocks=%u size=%u", s_card.block_count, s_card.block_size);
  tx_response(resp, pos);
}

static void handle_read_single_block(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;

  int blk_offset =
      (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  if (len < blk_offset + 1) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }

  uint8_t block = frame[blk_offset];
  if (block >= s_card.block_count) {
    tx_error(ISO15693_ERR_BLOCK_UNAVAILABLE);
    return;
  }

  const uint8_t *bdata = &s_card.mem[block * s_card.block_size];

  uint8_t resp[1 + ISO15693_MAX_BLOCK_SIZE];
  resp[0] = RESP_FLAGS_OK;
  memcpy(&resp[1], bdata, s_card.block_size);

  ESP_LOGI(TAG, "READ_SINGLE_BLOCK[%u]", block);
  tx_response(resp, 1 + (int)s_card.block_size);
}

static void handle_write_single_block(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;

  int blk_offset =
      (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  int data_offset = blk_offset + 1;

  if (len < data_offset + (int)s_card.block_size) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }

  uint8_t block = frame[blk_offset];
  if (block >= s_card.block_count) {
    tx_error(ISO15693_ERR_BLOCK_UNAVAILABLE);
    return;
  }
  if (s_block_locked[block]) {
    tx_error(ISO15693_ERR_BLOCK_LOCKED);
    return;
  }

  memcpy(&s_card.mem[block * s_card.block_size], &frame[data_offset], s_card.block_size);

  uint8_t resp[1] = {RESP_FLAGS_OK};
  ESP_LOGI(TAG, "WRITE_SINGLE_BLOCK[%u]", block);
  tx_response(resp, 1);
}

static void handle_read_multiple_blocks(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;

  int blk_offset =
      (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  if (len < blk_offset + 2) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }

  uint8_t first = frame[blk_offset];
  uint8_t count = (uint8_t)(frame[blk_offset + 1] + 1U);

  if ((unsigned)(first + count) > s_card.block_count) {
    tx_error(ISO15693_ERR_BLOCK_UNAVAILABLE);
    return;
  }

  static uint8_t resp[1 + ISO15693_EMU_MEM_SIZE];
  resp[0] = RESP_FLAGS_OK;
  size_t total = (size_t)count * s_card.block_size;
  memcpy(&resp[1], &s_card.mem[first * s_card.block_size], total);

  ESP_LOGI(TAG, "READ_MULTIPLE_BLOCKS[%u+%u]", first, count);
  tx_response(resp, (int)(1U + total));
}

static void handle_lock_block(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;

  int blk_offset =
      (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  if (len < blk_offset + 1) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }

  uint8_t block = frame[blk_offset];
  if (block >= s_card.block_count) {
    tx_error(ISO15693_ERR_BLOCK_UNAVAILABLE);
    return;
  }

  s_block_locked[block] = true;
  uint8_t resp[1] = {RESP_FLAGS_OK};
  ESP_LOGI(TAG, "LOCK_BLOCK[%u]", block);
  tx_response(resp, 1);
}

static void handle_write_afi(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;
  int off = (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  if (len < off + 1) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }
  if (s_afi_locked) {
    tx_error(ISO15693_ERR_LOCK_FAILED);
    return;
  }
  s_card.afi = frame[off];
  uint8_t resp[1] = {RESP_FLAGS_OK};
  ESP_LOGI(TAG, "WRITE_AFI=0x%02X", s_card.afi);
  tx_response(resp, 1);
}

static void handle_lock_afi(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;
  s_afi_locked = true;
  uint8_t resp[1] = {RESP_FLAGS_OK};
  ESP_LOGI(TAG, "LOCK_AFI");
  tx_response(resp, 1);
}

static void handle_write_dsfid(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;
  int off = (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  if (len < off + 1) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }
  if (s_dsfid_locked) {
    tx_error(ISO15693_ERR_LOCK_FAILED);
    return;
  }
  s_card.dsfid = frame[off];
  uint8_t resp[1] = {RESP_FLAGS_OK};
  ESP_LOGI(TAG, "WRITE_DSFID=0x%02X", s_card.dsfid);
  tx_response(resp, 1);
}

static void handle_lock_dsfid(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;
  s_dsfid_locked = true;
  uint8_t resp[1] = {RESP_FLAGS_OK};
  ESP_LOGI(TAG, "LOCK_DSFID");
  tx_response(resp, 1);
}

static void handle_get_multi_block_sec(const uint8_t *frame, int len) {
  if (!is_for_us(frame, len))
    return;

  int blk_offset =
      (frame[0] & ISO15693_FLAG_ADDRESS) ? ISO15693_ADDRESSED_DATA_OFF : ISO15693_DATA_OFF;
  if (len < blk_offset + 2) {
    tx_error(ISO15693_ERR_NOT_RECOGNIZED);
    return;
  }

  uint8_t first = frame[blk_offset];
  uint8_t count = (uint8_t)(frame[blk_offset + 1] + 1U);
  if ((unsigned)(first + count) > s_card.block_count) {
    tx_error(ISO15693_ERR_BLOCK_UNAVAILABLE);
    return;
  }

  uint8_t resp[1 + ISO15693_EMU_MAX_BLOCKS];
  resp[0] = RESP_FLAGS_OK;
  for (uint8_t i = 0; i < count; i++) {
    uint8_t block = (uint8_t)(first + i);
    resp[1 + i] = s_block_locked[block] ? 0x01U : 0x00U;
  }
  ESP_LOGI(TAG, "GET_MULTI_BLOCK_SEC[%u+%u]", first, count);
  tx_response(resp, 1 + count);
}

static void handle_stay_quiet(const uint8_t *frame, int len) {
  if (!uid_matches(frame, len, 2))
    return;
  ESP_LOGI(TAG, "STAY_QUIET -> silent until power cycle");
  s_quiet = true;
  s_state = ISO15693_STATE_SLEEP;
  hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
}

void iso15693_emu_card_from_tag(iso15693_emu_card_t *card,
                                const iso15693_tag_t *tag,
                                const uint8_t *raw_mem) {
  if (card == NULL || tag == NULL)
    return;
  memset(card, 0, sizeof(*card));
  memcpy(card->uid, tag->uid, ISO15693_UID_LEN);
  card->dsfid = tag->dsfid;
  card->afi = tag->afi;
  card->ic_ref = tag->ic_ref;
  card->block_count = tag->block_count;
  card->block_size = tag->block_size;
  if (raw_mem != NULL) {
    size_t mem_bytes = (size_t)tag->block_count * tag->block_size;
    if (mem_bytes > ISO15693_EMU_MEM_SIZE)
      mem_bytes = ISO15693_EMU_MEM_SIZE;
    memcpy(card->mem, raw_mem, mem_bytes);
  }
}

void iso15693_emu_card_default(iso15693_emu_card_t *card, const uint8_t uid[ISO15693_UID_LEN]) {
  if (card == NULL)
    return;
  memset(card, 0, sizeof(*card));
  memcpy(card->uid, uid, ISO15693_UID_LEN);
  card->dsfid = 0x00U;
  card->afi = 0x00U;
  card->ic_ref = 0x01U;
  card->block_count = ISO15693_EMU_DEFAULT_BLOCKS;
  card->block_size = ISO15693_EMU_DEFAULT_BSIZE;

  ESP_LOGI(TAG, "Default card: 8 blocks × 4 bytes");
}

hb_nfc_err_t iso15693_emu_init(const iso15693_emu_card_t *card) {
  if (card == NULL)
    return HB_NFC_ERR_PARAM;
  if (card->block_count > ISO15693_EMU_MAX_BLOCKS ||
      card->block_size > ISO15693_EMU_MAX_BLOCK_SIZE || card->block_count == 0 ||
      card->block_size == 0) {
    ESP_LOGE(TAG, "Invalid card params: blocks=%u size=%u", card->block_count, card->block_size);
    return HB_NFC_ERR_PARAM;
  }

  memcpy(&s_card, card, sizeof(s_card));
  s_state = ISO15693_STATE_SLEEP;
  s_quiet = false;
  s_init_done = true;
  memset(s_block_locked, 0, sizeof(s_block_locked));
  s_afi_locked = false;
  s_dsfid_locked = false;

  char uid_str[20];
  snprintf(uid_str,
           sizeof(uid_str),
           "%02X%02X%02X%02X%02X%02X%02X%02X",
           card->uid[7],
           card->uid[6],
           card->uid[5],
           card->uid[4],
           card->uid[3],
           card->uid[2],
           card->uid[1],
           card->uid[0]);
  ESP_LOGI(
      TAG, "Init: UID=%s  blocks=%u  block_size=%u", uid_str, card->block_count, card->block_size);
  return HB_NFC_OK;
}

hb_nfc_err_t iso15693_emu_configure_target(void) {
  if (!s_init_done)
    return HB_NFC_ERR_INTERNAL;

  ESP_LOGI(TAG, "Configure NFC-V target");

  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, ST25R3916_OP_CTRL_FIELD_OFF);
  vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_MEDIUM_MS));
  hb_nfc_spi_direct_cmd(ST25R3916_CMD_SET_DEFAULT);
  vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_LONG_MS));

  uint8_t ic = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_IC_IDENTITY, &ic);
  if (ic == 0x00U || ic == 0xFFU) {
    ESP_LOGE(TAG, "Chip not responding: IC=0x%02X", ic);
    return HB_NFC_ERR_INTERNAL;
  }

  hb_nfc_spi_reg_write(ST25R3916_REG_IO_CONF2, ISO15693_IO_CONF2_SUP_AAT);
  vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_SHORT_MS));
  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, ST25R3916_OP_CTRL_EN);
  wait_oscillator(ISO15693_OSC_TIMEOUT_MS);

  hb_nfc_spi_direct_cmd(ST25R3916_CMD_ADJUST_REGULATORS);
  vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_LONG_MS));

  hb_nfc_spi_reg_write(ST25R3916_REG_MODE, MODE_TARGET_NFCV);
  hb_nfc_spi_reg_write(ST25R3916_REG_BIT_RATE, 0x00U);
  hb_nfc_spi_reg_write(ST25R3916_REG_ISO14443B_FELICA, 0x00U);
  hb_nfc_spi_reg_write(ST25R3916_REG_PASSIVE_TARGET, 0x00U);

  hb_nfc_spi_reg_write(ST25R3916_REG_FIELD_THRESH_ACT, 0x00U);
  hb_nfc_spi_reg_write(ST25R3916_REG_FIELD_THRESH_DEACT, 0x00U);

  hb_nfc_spi_reg_write(ST25R3916_REG_PT_MOD, ISO15693_PT_MOD_OOK);

  {
    st25r3916_irq_status_t s;
    (void)st25r3916_irq_read(&s);
  }
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_MAIN_INT, 0x00U);
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_TIMER_NFC_INT, 0x00U);
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_ERROR_WUP_INT, 0x00U);
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_TARGET_INT, 0x00U);

  uint8_t mode_rb = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_MODE, &mode_rb);
  if (mode_rb != MODE_TARGET_NFCV) {
    ESP_LOGE(TAG, "MODE readback: 0x%02X (expected 0x%02X)", mode_rb, MODE_TARGET_NFCV);
    return HB_NFC_ERR_INTERNAL;
  }

  ESP_LOGI(TAG, "NFC-V target configured (MODE=0x%02X)", mode_rb);
  return HB_NFC_OK;
}

hb_nfc_err_t iso15693_emu_start(void) {
  if (!s_init_done)
    return HB_NFC_ERR_INTERNAL;

  {
    st25r3916_irq_status_t s;
    (void)st25r3916_irq_read(&s);
  }
  s_quiet = false;

  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, OP_CTRL_TARGET);
  vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_MEDIUM_MS));

  hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
  vTaskDelay(pdMS_TO_TICKS(ISO15693_DELAY_SHORT_MS));

  s_state = ISO15693_STATE_SLEEP;

  uint8_t op = 0, aux = 0, pts = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_OP_CTRL, &op);
  hb_nfc_spi_reg_read(ST25R3916_REG_AUX_DISPLAY, &aux);
  hb_nfc_spi_reg_read(ST25R3916_REG_PASSIVE_TARGET_STS, &pts);
  ESP_LOGI(TAG, "Start (SLEEP): OP=0x%02X AUX=0x%02X PT_STS=0x%02X", op, aux, pts);
  ESP_LOGI(TAG, "ISO 15693 emulation active — present the reader!");
  return HB_NFC_OK;
}

void iso15693_emu_stop(void) {
  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, ST25R3916_OP_CTRL_FIELD_OFF);
  s_state = ISO15693_STATE_SLEEP;
  s_init_done = false;
  ESP_LOGI(TAG, "ISO 15693 emulation stopped");
}

uint8_t *iso15693_emu_get_mem(void) {
  return s_card.mem;
}

void iso15693_emu_run_step(void) {
  uint8_t tgt_irq = 0;
  uint8_t main_irq = 0;
  uint8_t timer_irq = 0;

  hb_nfc_spi_reg_read(ST25R3916_REG_TARGET_INT, &tgt_irq);
  hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &main_irq);
  hb_nfc_spi_reg_read(ST25R3916_REG_TIMER_NFC_INT, &timer_irq);

  uint8_t pts = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_PASSIVE_TARGET_STS, &pts);
  uint8_t pta = pts & PTA_STATE_MASK;

  if (s_state == ISO15693_STATE_SLEEP) {
    bool wake = (timer_irq & TIMER_I_EON) || (tgt_irq & ST25R3916_IRQ_TGT_WU_A) ||
                (tgt_irq & ST25R3916_IRQ_TGT_WU_F);
    if (wake) {
      s_sense_idle = 0;
      s_active_idle = 0;
      ESP_LOGI(TAG, "SLEEP → SENSE (pta=%u tgt=0x%02X)", pta, tgt_irq);
      s_state = ISO15693_STATE_SENSE;
      hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SENSE);
      hb_nfc_spi_reg_read(ST25R3916_REG_TARGET_INT, &tgt_irq);
      hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &main_irq);
      hb_nfc_spi_reg_read(ST25R3916_REG_TIMER_NFC_INT, &timer_irq);
      if (!(main_irq & ST25R3916_IRQ_MAIN_RXE))
        return;
    } else {
      return;
    }
  }

  if (s_state == ISO15693_STATE_SENSE) {
    if (main_irq & ST25R3916_IRQ_MAIN_RXE) {
      s_sense_idle = 0;
      s_state = ISO15693_STATE_ACTIVE;
      /* Fall through to ACTIVE handling below */
    } else {
      if (++s_sense_idle > ISO15693_SENSE_IDLE_MAX) {
        ESP_LOGI(TAG, "SENSE: timeout -> SLEEP");
        s_sense_idle = 0;
        s_state = ISO15693_STATE_SLEEP;
        hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
      }
      return;
    }
  }

  if (!(main_irq & ST25R3916_IRQ_MAIN_RXE)) {
    if (++s_active_idle >= ACTIVE_IDLE_TIMEOUT) {
      ESP_LOGI(TAG, "ACTIVE: idle timeout -> SLEEP");
      s_active_idle = 0;
      s_state = ISO15693_STATE_SLEEP;
      hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
      return;
    }
    if ((s_active_idle % ISO15693_ACTIVE_LOG_INTERVAL) == 0U) {
      uint8_t aux = 0;
      hb_nfc_spi_reg_read(ST25R3916_REG_AUX_DISPLAY, &aux);
      ESP_LOGD(TAG, "[ACTIVE] AUX=0x%02X pta=%u", aux, pta);
    }
    return;
  }
  s_active_idle = 0;

  uint8_t buf[64] = {0};
  int len = fifo_rx(buf, sizeof(buf));
  if (len < 2)
    return;

  ESP_LOGI(TAG, "CMD: 0x%02X flags=0x%02X len=%d", buf[1], buf[0], len);

  uint8_t cmd_code = buf[1];

  switch (cmd_code) {
    case ISO15693_CMD_INVENTORY:
      handle_inventory(buf, len);
      break;

    case ISO15693_CMD_STAY_QUIET:
      handle_stay_quiet(buf, len);
      break;

    case ISO15693_CMD_GET_SYSTEM_INFO:
      handle_get_system_info(buf, len);
      break;

    case ISO15693_CMD_READ_SINGLE_BLOCK:
      handle_read_single_block(buf, len);
      break;

    case ISO15693_CMD_WRITE_SINGLE_BLOCK:
      handle_write_single_block(buf, len);
      break;

    case ISO15693_CMD_READ_MULTIPLE_BLOCKS:
      handle_read_multiple_blocks(buf, len);
      break;

    case ISO15693_CMD_LOCK_BLOCK:
      handle_lock_block(buf, len);
      break;

    case ISO15693_CMD_WRITE_AFI:
      handle_write_afi(buf, len);
      break;

    case ISO15693_CMD_LOCK_AFI:
      handle_lock_afi(buf, len);
      break;

    case ISO15693_CMD_WRITE_DSFID:
      handle_write_dsfid(buf, len);
      break;

    case ISO15693_CMD_LOCK_DSFID:
      handle_lock_dsfid(buf, len);
      break;

    case ISO15693_CMD_GET_MULTI_BLOCK_SEC:
      handle_get_multi_block_sec(buf, len);
      break;

    default:
      ESP_LOGW(TAG, "Unsupported cmd 0x%02X", cmd_code);
      if (is_for_us(buf, len)) {
        tx_error(ISO15693_ERR_NOT_SUPPORTED);
      }
      break;
  }
}
