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
 * @file t4t_emu.c
 * @brief ISO14443A Type 4 Tag emulation over ST25R3916 passive target mode.
 */
#include "t4t_emu.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "st25r3916_core.h"
#include "st25r3916_reg.h"
#include "st25r3916_cmd.h"
#include "st25r3916_fifo.h"
#include "st25r3916_irq.h"
#include "hb_nfc_spi.h"
#include "mf_desfire_emu.h"

static const char *TAG = "NFC_T4T_EMU";

#define OP_CTRL_TARGET             0xC3U
#define T4T_RX_IRQ                 (ST25R3916_IRQ_MAIN_FWL)
#define T4T_MAX_RESP_SIZE          260
#define T4T_CHAIN_BUF_SIZE         300
#define T4T_PT_MEM_SIZE            15
#define T4T_OSC_AUX_MASK           0x14U
#define T4T_OSC_MAIN_MASK          0x80U
#define T4T_OSC_TGT_MASK           0x08U
#define T4T_CC_V1                  0x00U
#define T4T_CC_MLEU_V1             0x0FU
#define T4T_CC_T                   0x20U
#define T4T_CC_NDEF_CTL            0x04U
#define T4T_CC_NDEF_RW             0x06U
#define T4T_NDEF_RECORD_HDR        0xD1U
#define T4T_NDEF_TNF_TYPE          0x01U
#define T4T_NDEF_TEXT_CODE         0x02U
#define T4T_NDEF_TEXT_PL_LEN       1
#define T4T_NDEF_LANG_LEN          2
#define T4T_PT_MEM_UID_OFFSET      0
#define T4T_PT_MEM_ATQA_OFFSET     10
#define T4T_PT_MEM_SAK_OFFSET      13
#define T4T_PCB_I_BLOCK            0x02U
#define T4T_PCB_CID_MASK           0x08U
#define T4T_PCB_TOGGLE             0x01U
#define T4T_PCB_R_ACK              0xA2U
#define T4T_DELAY_5MS              5
#define T4T_DELAY_10MS             10
#define T4T_DELAY_2MS              2
#define T4T_REG_AUX_DEF            0x10U
#define T4T_REG_IO_CONF2           0x80U
#define T4T_REG_PT_MOD             0x60U
#define T4T_IC_ID_INVALID          0x00U
#define T4T_IC_ID_FF               0xFFU
#define T4T_OSC_TIMEOUT            200
#define T4T_APDU_INS_SELECT        0xA4U
#define T4T_APDU_INS_READ          0xB0U
#define T4T_APDU_INS_UPDATE        0xD6U
#define T4T_APDU_P1_SELECT_AID     0x04U
#define T4T_APDU_P1_SELECT_FILE    0x00U
#define T4T_APDU_LC_AID            7
#define T4T_APDU_LC_FID            2
#define T4T_APDU_MIN_LEN           4
#define T4T_APDU_HDR_LEN           5
#define T4T_APDU_SW_ERR_SYNTAX     0x6700U
#define T4T_APDU_SW_OK             0x9000U
#define T4T_APDU_SW_FILE_NOT_FOUND 0x6A82U
#define T4T_APDU_SW_SEC_ERROR      0x6985U
#define T4T_APDU_SW_LE_INVALID     0x6C00U
#define T4T_APDU_SW_EOF            0x6B00U
#define T4T_APDU_LE_MAX            256
#define T4T_PTA_STATE_IDLE         0x01U
#define T4T_PTA_STATE_ACTIVE       0x05U
#define T4T_PTA_SENSE_TIMEOUT      500
#define T4T_PTA_ACTIVE_TIMEOUT     2000
#define T4T_PTA_IDLE_CHECK         500
#define T4T_PTA_SENSE_CHECK        200
#define T4T_PCB_CLASS_MASK         0xC0U
#define T4T_PCB_I_CLASS            0x00U
#define T4T_PCB_R_CLASS            0x80U
#define T4T_PCB_S_CLASS            0xC0U
#define T4T_PCB_CHAIN_MASK         0x10U
#define T4T_PCB_NAD_MASK           0x04U
#define T4T_FS1_LEN_MASK           0x7FU
#define T4T_CMD_RATS               0xE0U
#define T4T_CMD_PPS_MASK           0xF0U
#define T4T_CMD_PPS_BASE           0xD0U
#define T4T_FIFO_STATUS_BITS       7
#define T4T_DELAY_1MS              1
#define T4T_PT_MEM_CT_OFFSET       12
#define T4T_PT_MEM_CT_BYTE         0x04U
#define T4T_OP_CTRL_OFF            0x00U
#define T4T_OP_CTRL_ON             0x80U
#define T4T_APDU_SW_WRONG_P1P2     0x6A86U
#define T4T_PTA_STATE_MASK         0x0FU
#define T4T_PTA_STATE_WUPA         0x02U
#define T4T_PTA_STATE_SELECTED     0x03U
#define T4T_PTA_STATE_HALTED       0x0DU

static const uint8_t k_uid7[7] = {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};
static const uint8_t k_atqa[2] = {0x44, 0x00};
static const uint8_t k_sak = 0x20;

static const uint8_t k_ats[] = {0x02, 0x06};
static const uint8_t k_ndef_aid[] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};

#define T4T_CC_LEN   15
#define T4T_NDEF_MAX 128
#define T4T_FILE_MAX (2 + T4T_NDEF_MAX)
#define T4T_ML       0x0050U
#define T4T_NDEF_FID 0xE104U
#define T4T_CC_FID   0xE103U

typedef enum {
  T4T_STATE_SLEEP,
  T4T_STATE_SENSE,
  T4T_STATE_ACTIVE,
} t4t_state_t;

typedef enum {
  T4T_FILE_NONE = 0,
  T4T_FILE_CC,
  T4T_FILE_NDEF,
} t4t_file_t;

static bool wait_oscillator(int timeout_ms);
static hb_nfc_err_t load_pt_memory(void);
static void build_cc(void);
static void build_ndef_text(const char *text);
static void tx_with_crc(const uint8_t *data, int len);
static void send_i_block_resp(const uint8_t *inf, int inf_len, uint8_t pcb_in);
static void send_r_ack(uint8_t pcb_in);
static void
apdu_resp(uint8_t *out_buffer, int *out_len, const uint8_t *data, int data_len, uint16_t sw);
static uint16_t file_len(t4t_file_t f);
static uint8_t *file_ptr(t4t_file_t f);
static void handle_apdu(const uint8_t *apdu, int apdu_len, uint8_t pcb_in);

static t4t_state_t s_state = T4T_STATE_SLEEP;
static bool s_init_done = false;

static bool s_app_selected = false;
static t4t_file_t s_file = T4T_FILE_NONE;

static uint8_t s_cc[T4T_CC_LEN];
static uint8_t s_ndef_file[T4T_FILE_MAX];

static uint8_t s_last_resp[T4T_MAX_RESP_SIZE];
static int s_last_resp_len = 0;

static uint8_t s_chain_buf[T4T_CHAIN_BUF_SIZE];
static size_t s_chain_len = 0;
static bool s_chain_active = false;
static uint8_t s_pcd_cid = 0;
static TickType_t s_sense_tick = 0;
static TickType_t s_active_tick = 0;

static bool wait_oscillator(int timeout_ms) {
  for (int i = 0; i < timeout_ms; i++) {
    uint8_t aux = 0, mi = 0, ti = 0;
    hb_nfc_spi_reg_read(ST25R3916_REG_AUX_DISPLAY, &aux);
    hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &mi);
    hb_nfc_spi_reg_read(ST25R3916_REG_TARGET_INT, &ti);
    if (((aux & T4T_OSC_AUX_MASK) != 0) || ((mi & T4T_OSC_MAIN_MASK) != 0) ||
        ((ti & T4T_OSC_TGT_MASK) != 0)) {
      ESP_LOGI(TAG, "Osc OK in %dms: AUX=0x%02X MAIN=0x%02X TGT=0x%02X", i, aux, mi, ti);
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_1MS));
  }
  ESP_LOGW(TAG, "Osc timeout - continuing");
  return false;
}

static hb_nfc_err_t load_pt_memory(void) {
  uint8_t ptm[T4T_PT_MEM_SIZE] = {0};
  ptm[0] = k_uid7[0];
  ptm[1] = k_uid7[1];
  ptm[2] = k_uid7[2];
  ptm[3] = k_uid7[3];
  ptm[4] = k_uid7[4];
  ptm[5] = k_uid7[5];
  ptm[6] = k_uid7[6];
  ptm[7] = 0x00;
  ptm[8] = 0x00;
  ptm[9] = 0x00;
  ptm[T4T_PT_MEM_ATQA_OFFSET] = k_atqa[0];
  ptm[T4T_PT_MEM_ATQA_OFFSET + 1] = k_atqa[1];
  ptm[T4T_PT_MEM_CT_OFFSET] = T4T_PT_MEM_CT_BYTE;
  ptm[T4T_PT_MEM_SAK_OFFSET] = k_sak;
  ptm[14] = 0x00;

  hb_nfc_err_t err = hb_nfc_spi_pt_mem_write(ST25R3916_SPI_PT_MEM_A_WRITE, ptm, T4T_PT_MEM_SIZE);
  if (err != HB_NFC_OK)
    return err;
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_2MS));

  uint8_t rb[T4T_PT_MEM_SIZE] = {0};
  err = hb_nfc_spi_pt_mem_read(rb, T4T_PT_MEM_SIZE);
  if (err != HB_NFC_OK)
    return err;
  if (memcmp(ptm, rb, T4T_PT_MEM_SIZE) != 0) {
    ESP_LOGE(TAG, "PT Memory mismatch!");
    return HB_NFC_ERR_INTERNAL;
  }
  ESP_LOGI(TAG,
           "PT OK: UID=%02X%02X%02X%02X%02X%02X%02X ATQA=%02X%02X SAK=%02X",
           ptm[0],
           ptm[1],
           ptm[2],
           ptm[3],
           ptm[4],
           ptm[5],
           ptm[6],
           ptm[10],
           ptm[11],
           ptm[13]);
  return HB_NFC_OK;
}

static void build_cc(void) {
  s_cc[0] = T4T_CC_V1;
  s_cc[1] = T4T_CC_MLEU_V1;
  s_cc[2] = T4T_CC_T;
  s_cc[3] = (uint8_t)((T4T_ML >> 8) & 0xFF);
  s_cc[4] = (uint8_t)(T4T_ML & 0xFF);
  s_cc[5] = (uint8_t)((T4T_ML >> 8) & 0xFF);
  s_cc[6] = (uint8_t)(T4T_ML & 0xFF);
  s_cc[7] = T4T_CC_NDEF_CTL;
  s_cc[8] = T4T_CC_NDEF_RW;
  s_cc[9] = (uint8_t)((T4T_NDEF_FID >> 8) & 0xFF);
  s_cc[10] = (uint8_t)(T4T_NDEF_FID & 0xFF);
  s_cc[11] = 0x00;
  s_cc[12] = (uint8_t)T4T_NDEF_MAX;
  s_cc[13] = 0x00;
  s_cc[14] = 0x00;
}

static void build_ndef_text(const char *text) {
  memset(s_ndef_file, 0x00, sizeof(s_ndef_file));

  if (text == NULL)
    text = "High Boy NFC T4T";
  size_t tl = strlen(text);
  size_t max_text = (T4T_NDEF_MAX > 7) ? (T4T_NDEF_MAX - 7) : 0;
  if (tl > max_text)
    tl = max_text;
  size_t pl = T4T_NDEF_TEXT_PL_LEN + T4T_NDEF_LANG_LEN + tl;

  size_t pos = 2;
  s_ndef_file[pos++] = T4T_NDEF_RECORD_HDR;
  s_ndef_file[pos++] = T4T_NDEF_TNF_TYPE;
  s_ndef_file[pos++] = (uint8_t)pl;
  s_ndef_file[pos++] = 'T';
  s_ndef_file[pos++] = T4T_NDEF_TEXT_CODE;
  s_ndef_file[pos++] = 'p';
  s_ndef_file[pos++] = 't';

  memcpy(&s_ndef_file[pos], text, tl);
  pos += tl;

  uint16_t nlen = (uint16_t)(pos - 2);
  s_ndef_file[0] = (uint8_t)((nlen >> 8) & 0xFF);
  s_ndef_file[1] = (uint8_t)(nlen & 0xFF);
}

hb_nfc_err_t t4t_emu_init_default(void) {
  build_cc();
  build_ndef_text("High Boy NFC T4T");
  s_state = T4T_STATE_SLEEP;
  s_init_done = true;
  s_app_selected = false;
  s_file = T4T_FILE_NONE;
  s_chain_active = false;
  s_chain_len = 0;
  s_pcd_cid = 0;
  s_sense_tick = 0;
  s_active_tick = 0;
  ESP_LOGI(TAG, "T4T init: NDEF max=%u", (unsigned)T4T_NDEF_MAX);
  return HB_NFC_OK;
}

hb_nfc_err_t t4t_emu_configure_target(void) {
  if (!s_init_done)
    return HB_NFC_ERR_INTERNAL;

  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, T4T_OP_CTRL_OFF);
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_5MS));
  hb_nfc_spi_direct_cmd(ST25R3916_CMD_SET_DEFAULT);
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_10MS));

  uint8_t ic = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_IC_IDENTITY, &ic);
  if (ic == T4T_IC_ID_INVALID || ic == T4T_IC_ID_FF)
    return HB_NFC_ERR_INTERNAL;

  hb_nfc_spi_reg_write(ST25R3916_REG_IO_CONF2, T4T_REG_IO_CONF2);
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_2MS));

  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, T4T_OP_CTRL_ON);
  wait_oscillator(T4T_OSC_TIMEOUT);

  hb_nfc_spi_direct_cmd(ST25R3916_CMD_ADJUST_REGULATORS);
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_10MS));

  hb_nfc_spi_reg_write(ST25R3916_REG_MODE, ST25R3916_MODE_TARGET_NFCA);
  hb_nfc_spi_reg_write(ST25R3916_REG_AUX_DEF, T4T_REG_AUX_DEF);
  hb_nfc_spi_reg_write(ST25R3916_REG_BIT_RATE, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_ISO14443A, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_PASSIVE_TARGET, 0x00);

  hb_nfc_spi_reg_write(ST25R3916_REG_FIELD_THRESH_ACT, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_FIELD_THRESH_DEACT, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_PT_MOD, T4T_REG_PT_MOD);

  hb_nfc_err_t err = load_pt_memory();
  if (err != HB_NFC_OK)
    return err;

  {
    st25r3916_irq_status_t s;
    (void)st25r3916_irq_read(&s);
  }
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_MAIN_INT, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_TIMER_NFC_INT, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_ERROR_WUP_INT, 0x00);
  hb_nfc_spi_reg_write(ST25R3916_REG_MASK_TARGET_INT, 0x00);

  ESP_LOGI(TAG, "T4T target configured");
  return HB_NFC_OK;
}

hb_nfc_err_t t4t_emu_start(void) {
  if (!s_init_done)
    return HB_NFC_ERR_INTERNAL;

  {
    st25r3916_irq_status_t s;
    (void)st25r3916_irq_read(&s);
  }

  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, OP_CTRL_TARGET);
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_5MS));

  hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
  vTaskDelay(pdMS_TO_TICKS(T4T_DELAY_2MS));

  s_state = T4T_STATE_SLEEP;
  ESP_LOGI(TAG, "T4T emulation active - present a reader");
  return HB_NFC_OK;
}

void t4t_emu_stop(void) {
  hb_nfc_spi_reg_write(ST25R3916_REG_OP_CTRL, T4T_OP_CTRL_OFF);
  s_state = T4T_STATE_SLEEP;
  s_init_done = false;
  ESP_LOGI(TAG, "T4T emulation stopped");
}

static void tx_with_crc(const uint8_t *data, int len) {
  if (data == NULL || len <= 0)
    return;
  st25r3916_fifo_clear();
  st25r3916_fifo_set_tx_bytes((uint16_t)len, 0);
  st25r3916_fifo_load(data, (size_t)len);
  hb_nfc_spi_direct_cmd(ST25R3916_CMD_TX_WITH_CRC);
}

static void send_i_block_resp(const uint8_t *inf, int inf_len, uint8_t pcb_in) {
  uint8_t resp[T4T_MAX_RESP_SIZE];
  int pos = 0;
  resp[pos++] = (uint8_t)(T4T_PCB_I_BLOCK | (pcb_in & T4T_PCB_TOGGLE));
  if (pcb_in & T4T_PCB_CID_MASK) {
    resp[pos++] = s_pcd_cid;
  }
  if (inf_len > 0 && inf_len < (int)sizeof(resp) - 1) {
    memcpy(&resp[pos], inf, (size_t)inf_len);
    pos += inf_len;
  }
  tx_with_crc(resp, pos);
  if (pos < (int)sizeof(s_last_resp)) {
    memcpy(s_last_resp, resp, (size_t)pos);
    s_last_resp_len = pos;
  }
}

static void send_r_ack(uint8_t pcb_in) {
  uint8_t resp[2];
  int pos = 0;
  resp[pos++] = (uint8_t)(T4T_PCB_R_ACK | (pcb_in & T4T_PCB_TOGGLE));
  if (pcb_in & T4T_PCB_CID_MASK) {
    resp[pos++] = s_pcd_cid;
  }
  tx_with_crc(resp, pos);
}

static void
apdu_resp(uint8_t *out_buffer, int *out_len, const uint8_t *data, int data_len, uint16_t sw) {
  int pos = 0;
  if (data != NULL && data_len > 0) {
    memcpy(&out_buffer[pos], data, (size_t)data_len);
    pos += data_len;
  }
  out_buffer[pos++] = (uint8_t)((sw >> 8) & 0xFF);
  out_buffer[pos++] = (uint8_t)(sw & 0xFF);
  *out_len = pos;
}

static uint16_t file_len(t4t_file_t f) {
  if (f == T4T_FILE_CC)
    return T4T_CC_LEN;
  if (f == T4T_FILE_NDEF) {
    uint16_t nlen = (uint16_t)((s_ndef_file[0] << 8) | s_ndef_file[1]);
    if (nlen > T4T_NDEF_MAX)
      nlen = T4T_NDEF_MAX;
    return (uint16_t)(nlen + 2);
  }
  return 0;
}

static uint8_t *file_ptr(t4t_file_t f) {
  if (f == T4T_FILE_CC)
    return s_cc;
  if (f == T4T_FILE_NDEF)
    return s_ndef_file;
  return NULL;
}

static void handle_apdu(const uint8_t *apdu, int apdu_len, uint8_t pcb_in) {
  uint8_t resp[T4T_MAX_RESP_SIZE];
  int resp_len = 0;

  if (mf_desfire_emu_handle_apdu(apdu, apdu_len, resp, &resp_len)) {
    send_i_block_resp(resp, resp_len, pcb_in);
    return;
  }

  if (apdu_len < T4T_APDU_MIN_LEN) {
    apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
    send_i_block_resp(resp, resp_len, pcb_in);
    return;
  }

  uint8_t cla = apdu[0];
  uint8_t ins = apdu[1];
  uint8_t p1 = apdu[2];
  uint8_t p2 = apdu[3];
  (void)cla;

  if (ins == T4T_APDU_INS_SELECT) {
    if (apdu_len < T4T_APDU_HDR_LEN) {
      apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
    } else {
      uint8_t lc = apdu[4];
      if (apdu_len < T4T_APDU_HDR_LEN + lc) {
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
      } else if (p1 == T4T_APDU_P1_SELECT_AID && lc == T4T_APDU_LC_AID &&
                 memcmp(&apdu[5], k_ndef_aid, T4T_APDU_LC_AID) == 0) {
        s_app_selected = true;
        s_file = T4T_FILE_NONE;
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_OK);
      } else if (p1 == T4T_APDU_P1_SELECT_FILE && lc == T4T_APDU_LC_FID) {
        uint16_t fid = (uint16_t)((apdu[5] << 8) | apdu[6]);
        if (fid == T4T_CC_FID) {
          s_file = T4T_FILE_CC;
          apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_OK);
        } else if (fid == T4T_NDEF_FID) {
          s_file = T4T_FILE_NDEF;
          apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_OK);
        } else {
          apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_FILE_NOT_FOUND);
        }
      } else {
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_FILE_NOT_FOUND);
      }
    }
    send_i_block_resp(resp, resp_len, pcb_in);
    return;
  }

  if (ins == T4T_APDU_INS_READ) {
    if (apdu_len < T4T_APDU_HDR_LEN) {
      apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
    } else if (s_file == T4T_FILE_NONE || !s_app_selected) {
      apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_SEC_ERROR);
    } else {
      uint16_t off = (uint16_t)((p1 << 8) | p2);
      uint16_t flen = file_len(s_file);
      uint8_t le = apdu[4];
      uint16_t max_le = (le == 0) ? T4T_APDU_LE_MAX : le;
      if (max_le > T4T_ML) {
        apdu_resp(resp, &resp_len, NULL, 0, (uint16_t)(T4T_APDU_SW_LE_INVALID | (T4T_ML & 0xFF)));
        send_i_block_resp(resp, resp_len, pcb_in);
        return;
      }
      if (off >= flen) {
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_EOF);
      } else {
        uint16_t avail = (uint16_t)(flen - off);
        uint16_t rd = (avail < max_le) ? avail : max_le;
        uint8_t *fp = file_ptr(s_file);
        apdu_resp(resp, &resp_len, &fp[off], rd, T4T_APDU_SW_OK);
      }
    }
    send_i_block_resp(resp, resp_len, pcb_in);
    return;
  }

  if (ins == T4T_APDU_INS_UPDATE) {
    if (apdu_len < T4T_APDU_HDR_LEN) {
      apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
    } else if (s_file != T4T_FILE_NDEF || !s_app_selected) {
      apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_SEC_ERROR);
    } else {
      uint16_t off = (uint16_t)((p1 << 8) | p2);
      uint8_t lc = apdu[4];
      if (apdu_len < T4T_APDU_HDR_LEN + lc) {
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
      } else if (lc > T4T_ML) {
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_ERR_SYNTAX);
      } else if (off + lc > (uint16_t)sizeof(s_ndef_file)) {
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_EOF);
      } else {
        memcpy(&s_ndef_file[off], &apdu[5], lc);
        apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_OK);
      }
    }
    send_i_block_resp(resp, resp_len, pcb_in);
    return;
  }

  apdu_resp(resp, &resp_len, NULL, 0, T4T_APDU_SW_WRONG_P1P2);
  send_i_block_resp(resp, resp_len, pcb_in);
}

void t4t_emu_run_step(void) {
  uint8_t tgt_irq = 0;
  uint8_t main_irq = 0;
  uint8_t timer_irq = 0;

  hb_nfc_spi_reg_read(ST25R3916_REG_TARGET_INT, &tgt_irq);
  hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &main_irq);
  hb_nfc_spi_reg_read(ST25R3916_REG_TIMER_NFC_INT, &timer_irq);

  uint8_t pts = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_PASSIVE_TARGET_STS, &pts);
  uint8_t pta_state = pts & T4T_PTA_STATE_MASK;

  if (s_state == T4T_STATE_SLEEP) {
    if (pta_state == T4T_PTA_STATE_IDLE || pta_state == T4T_PTA_STATE_WUPA ||
        pta_state == T4T_PTA_STATE_SELECTED || (tgt_irq & ST25R3916_IRQ_TGT_WU_A)) {
      ESP_LOGI(TAG, "SLEEP -> SENSE (pta=%u tgt=0x%02X)", pta_state, tgt_irq);
      s_sense_tick = xTaskGetTickCount();
      s_active_tick = s_sense_tick;
      s_state = T4T_STATE_SENSE;
      mf_desfire_emu_reset();
      hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SENSE);
      hb_nfc_spi_reg_read(ST25R3916_REG_TARGET_INT, &tgt_irq);
      hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &main_irq);
      hb_nfc_spi_reg_read(ST25R3916_REG_TIMER_NFC_INT, &timer_irq);
    }
    return;
  }

  if (s_state == T4T_STATE_SENSE) {
    if (pta_state == T4T_PTA_STATE_ACTIVE || pta_state == T4T_PTA_STATE_HALTED ||
        (tgt_irq & ST25R3916_IRQ_TGT_SDD_C)) {
      ESP_LOGI(TAG, "SENSE -> ACTIVE (pta=%u tgt=0x%02X)", pta_state, tgt_irq);
      s_state = T4T_STATE_ACTIVE;
      s_active_tick = xTaskGetTickCount();
    } else {
      TickType_t now = xTaskGetTickCount();
      if ((now - s_sense_tick) > pdMS_TO_TICKS(T4T_PTA_SENSE_TIMEOUT)) {
        ESP_LOGI(TAG, "SENSE: idle timeout -> SLEEP");
        s_state = T4T_STATE_SLEEP;
        s_app_selected = false;
        s_file = T4T_FILE_NONE;
        s_chain_active = false;
        s_chain_len = 0;
        s_pcd_cid = 0;
        mf_desfire_emu_reset();
        hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
        return;
      }
      if (((now - s_sense_tick) % pdMS_TO_TICKS(T4T_PTA_SENSE_CHECK)) == 0U) {
        uint8_t aux = 0;
        hb_nfc_spi_reg_read(ST25R3916_REG_AUX_DISPLAY, &aux);
        ESP_LOGD(TAG, "[SENSE] AUX=0x%02X pta=%u", aux, pta_state);
      }
      return;
    }
  }

  if (!(main_irq & T4T_RX_IRQ))
    goto idle_active;

  uint8_t buf[T4T_CHAIN_BUF_SIZE] = {0};
  uint8_t fs1 = 0;
  hb_nfc_spi_reg_read(ST25R3916_REG_FIFO_STATUS1, &fs1);
  int n = fs1 & T4T_FIFO_STATUS_BITS;
  if (n <= 0)
    return;
  if (n > (int)sizeof(buf))
    n = (int)sizeof(buf);
  hb_nfc_spi_fifo_read(buf, (uint8_t)n);
  int len = n;

  if (len <= 0)
    return;

  if (buf[0] == T4T_CMD_RATS && len >= 2) {
    ESP_LOGI(TAG, "RATS");
    tx_with_crc(k_ats, sizeof(k_ats));
    return;
  }

  if ((buf[0] & T4T_CMD_PPS_MASK) == T4T_CMD_PPS_BASE) {
    ESP_LOGI(TAG, "PPS");
    uint8_t resp = buf[0];
    tx_with_crc(&resp, 1);
    return;
  }

  uint8_t pcb = buf[0];
  if ((pcb & T4T_PCB_CLASS_MASK) == T4T_PCB_R_CLASS) {
    if (s_last_resp_len > 0) {
      tx_with_crc(s_last_resp, s_last_resp_len);
    }
    return;
  }

  if ((pcb & T4T_PCB_CLASS_MASK) != T4T_PCB_I_CLASS) {
    return;
  }

  bool chaining = (pcb & T4T_PCB_CHAIN_MASK) != 0U;
  int idx = 1;
  if (pcb & T4T_PCB_CID_MASK) {
    s_pcd_cid = buf[idx];
    idx++;
  }
  if (pcb & T4T_PCB_NAD_MASK) {
    idx++;
  }
  if (idx > len)
    return;
  const uint8_t *inf = &buf[idx];
  int inf_len = len - idx;

  if (chaining) {
    if (!s_chain_active) {
      s_chain_active = true;
      s_chain_len = 0;
    }
    if (s_chain_len + (size_t)inf_len < sizeof(s_chain_buf)) {
      memcpy(&s_chain_buf[s_chain_len], inf, (size_t)inf_len);
      s_chain_len += (size_t)inf_len;
    }
    send_r_ack(pcb);
    return;
  }

  if (s_chain_active) {
    if (s_chain_len + (size_t)inf_len < sizeof(s_chain_buf)) {
      memcpy(&s_chain_buf[s_chain_len], inf, (size_t)inf_len);
      s_chain_len += (size_t)inf_len;
    }
    ESP_LOGI(TAG, "APDU (chained) len=%u", (unsigned)s_chain_len);
    handle_apdu(s_chain_buf, (int)s_chain_len, pcb);
    s_chain_active = false;
    s_chain_len = 0;
    return;
  }

  ESP_LOGI(TAG, "APDU len=%d", inf_len);
  handle_apdu(inf, inf_len, pcb);
  s_active_tick = xTaskGetTickCount();
  return;

idle_active: {
  TickType_t now = xTaskGetTickCount();
  if ((now - s_active_tick) > pdMS_TO_TICKS(T4T_PTA_ACTIVE_TIMEOUT)) {
    ESP_LOGI(TAG, "ACTIVE: idle timeout -> SLEEP");
    s_state = T4T_STATE_SLEEP;
    s_app_selected = false;
    s_file = T4T_FILE_NONE;
    s_chain_active = false;
    s_chain_len = 0;
    s_pcd_cid = 0;
    mf_desfire_emu_reset();
    hb_nfc_spi_direct_cmd(ST25R3916_CMD_GOTO_SLEEP);
  } else if (((now - s_active_tick) % pdMS_TO_TICKS(T4T_PTA_IDLE_CHECK)) == 0U) {
    uint8_t aux = 0;
    hb_nfc_spi_reg_read(ST25R3916_REG_AUX_DISPLAY, &aux);
    ESP_LOGD(TAG, "[ACTIVE] AUX=0x%02X pta=%u", aux, pta_state);
  }
}
}
