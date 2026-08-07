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

#include "nfc_apdu.h"

#include <string.h>

#include "esp_log.h"

#include "nfc_tcl.h"

static const char *TAG = "NFC_APDU";

#define APDU_MAX_BUF                  300U
#define APDU_STATUS_WORD_SIZE         2U    /**< Status word is always 2 bytes */
#define APDU_MIN_RESPONSE_SIZE        2U    /**< Minimum response is just SW */
#define APDU_SELECT_AID_P2            0x04U /**< P2: select by name (AID) */
#define APDU_SELECT_FILE_P1           0x00U /**< P1: select by file ID */
#define APDU_SELECT_FILE_P2           0x0CU /**< P2: select by file ID */
#define APDU_SELECT_FILE_LC           0x02U /**< Lc for FILE ID (2 bytes) */
#define APDU_SELECT_FILE_MIN_SIZE     7U    /**< Minimum APDU size for SELECT FILE */
#define APDU_SELECT_FILE_SIZE_WITH_LE 8U    /**< SELECT FILE APDU size with Le */
#define APDU_BYTE_HIGH_SHIFT          8U    /**< Shift for high byte in 16-bit value */
#define APDU_BYTE_MASK                0xFFU /**< Mask for single byte */
#define APDU_STATUS_WORD_HIGH_INDEX   (APDU_STATUS_WORD_SIZE - 2) /**< High byte index of SW */
#define APDU_STATUS_WORD_LOW_INDEX    (APDU_STATUS_WORD_SIZE - 1) /**< Low byte index of SW */
#define APDU_LE_UNSPECIFIED           0x00U /**< Le = 0 means return all available */

const char *nfc_apdu_sw_str(uint16_t sw) {
  switch (sw) {
    case APDU_SW_OK:
      return "OK";
    case APDU_SW_WRONG_LENGTH:
      return "Wrong length";
    case APDU_SW_SECURITY:
      return "Security status not satisfied";
    case APDU_SW_AUTH_FAILED:
      return "Authentication failed";
    case APDU_SW_FILE_NOT_FOUND:
      return "File not found";
    case APDU_SW_INS_NOT_SUP:
      return "INS not supported";
    case APDU_SW_CLA_NOT_SUP:
      return "CLA not supported";
    case APDU_SW_UNKNOWN:
      return "Unknown error";
    default:
      return "SW error";
  }
}

hb_nfc_err_t nfc_apdu_sw_to_err(uint16_t sw) {
  if (sw == APDU_SW_OK)
    return HB_NFC_OK;
  if (sw == APDU_SW_SECURITY || sw == APDU_SW_AUTH_FAILED)
    return HB_NFC_ERR_AUTH;
  return HB_NFC_ERR_PROTOCOL;
}

static int apdu_transceive_raw(const nfc_apdu_channel_t *ch,
                               const uint8_t *apdu,
                               size_t apdu_len,
                               uint8_t *rx,
                               size_t rx_max) {
  return nfc_tcl_transceive(ch->proto, ch->ctx, apdu, apdu_len, rx, rx_max, ch->timeout_ms);
}

static int apdu_get_response(const nfc_apdu_channel_t *ch, uint8_t le, uint8_t *rx, size_t rx_max) {
  uint8_t apdu[APDU_HEADER_SIZE] = {
      APDU_CLA_ISO7816, APDU_INS_GET_RESPONSE, APDU_SELECT_FILE_P1, APDU_SELECT_FILE_P1, le};
  return apdu_transceive_raw(ch, apdu, sizeof(apdu), rx, rx_max);
}

hb_nfc_err_t nfc_apdu_transceive(const nfc_apdu_channel_t *ch,
                                 const uint8_t *apdu,
                                 size_t apdu_len,
                                 nfc_apdu_resp_t *resp) {
  if (!ch || !ch->ctx || !apdu || apdu_len == 0 || !resp || !resp->buf ||
      resp->max < APDU_STATUS_WORD_SIZE)
    return HB_NFC_ERR_PARAM;

  uint8_t cmd[APDU_MAX_BUF];
  if (apdu_len > sizeof(cmd))
    return HB_NFC_ERR_PARAM;
  memcpy(cmd, apdu, apdu_len);

  bool sw_in_buf = true;

  int len = apdu_transceive_raw(ch, cmd, apdu_len, resp->buf, resp->max);
  if (len < APDU_STATUS_WORD_SIZE)
    return HB_NFC_ERR_PROTOCOL;

  uint16_t status =
      (uint16_t)((resp->buf[len - APDU_STATUS_WORD_HIGH_INDEX] << APDU_BYTE_HIGH_SHIFT) |
                 resp->buf[len - APDU_STATUS_WORD_LOW_INDEX]);

  if (((status & APDU_SW_CLASS_MASK) == APDU_SW_WRONG_LE) && apdu_len >= APDU_HEADER_SIZE) {
    cmd[apdu_len - 1] = (uint8_t)(status & APDU_SW_DATA_MASK);
    len = apdu_transceive_raw(ch, cmd, apdu_len, resp->buf, resp->max);
    if (len < APDU_STATUS_WORD_SIZE)
      return HB_NFC_ERR_PROTOCOL;
    status = (uint16_t)((resp->buf[len - APDU_STATUS_WORD_HIGH_INDEX] << APDU_BYTE_HIGH_SHIFT) |
                        resp->buf[len - APDU_STATUS_WORD_LOW_INDEX]);
  }

  if ((status & APDU_SW_CLASS_MASK) == APDU_SW_MORE_DATA) {
    size_t out_pos = (size_t)(len - APDU_STATUS_WORD_SIZE);
    uint8_t le = (uint8_t)(status & APDU_SW_DATA_MASK);
    if (le == 0)
      le = APDU_LE_UNSPECIFIED;

    for (int i = 0; i < APDU_MAX_GET_RESP; i++) {
      uint8_t tmp[APDU_MAX_BUF];
      int rlen = apdu_get_response(ch, le, tmp, sizeof(tmp));
      if (rlen < APDU_STATUS_WORD_SIZE)
        break;

      uint16_t swr = (uint16_t)((tmp[rlen - APDU_STATUS_WORD_HIGH_INDEX] << APDU_BYTE_HIGH_SHIFT) |
                                tmp[rlen - APDU_STATUS_WORD_LOW_INDEX]);
      int data_len = rlen - APDU_STATUS_WORD_SIZE;
      if (data_len > 0) {
        size_t copy = (out_pos + (size_t)data_len <= resp->max)
                          ? (size_t)data_len
                          : (resp->max > out_pos ? (resp->max - out_pos) : 0);
        if (copy > 0) {
          memcpy(&resp->buf[out_pos], tmp, copy);
          out_pos += copy;
        }
      }

      status = swr;
      if ((status & APDU_SW_CLASS_MASK) != APDU_SW_MORE_DATA)
        break;
      le = (uint8_t)(status & APDU_SW_DATA_MASK);
      if (le == 0)
        le = APDU_LE_UNSPECIFIED;
    }

    if (out_pos + APDU_STATUS_WORD_SIZE <= resp->max) {
      resp->buf[out_pos++] = (uint8_t)(status >> APDU_BYTE_HIGH_SHIFT);
      resp->buf[out_pos++] = (uint8_t)(status & APDU_SW_DATA_MASK);
      sw_in_buf = true;
    } else {
      sw_in_buf = false;
    }
    len = (int)out_pos;
  }

  resp->sw = status;
  resp->len = sw_in_buf ? (size_t)(len - APDU_STATUS_WORD_SIZE) : (size_t)len;
  return nfc_apdu_sw_to_err(status);
}

hb_nfc_err_t nfc_apdu_send(const nfc_apdu_channel_t *ch,
                           const uint8_t *apdu,
                           size_t apdu_len,
                           nfc_apdu_resp_t *resp) {
  return nfc_apdu_transceive(ch, apdu, apdu_len, resp);
}

hb_nfc_err_t nfc_apdu_recv(const nfc_apdu_channel_t *ch,
                           const uint8_t *apdu,
                           size_t apdu_len,
                           nfc_apdu_resp_t *resp) {
  return nfc_apdu_transceive(ch, apdu, apdu_len, resp);
}

size_t
nfc_apdu_build_select_aid(nfc_apdu_buf_t dst, nfc_apdu_data_t aid, bool le_present, uint8_t le) {
  if (!dst.buf || !aid.ptr || aid.len == 0)
    return 0;
  if (dst.max < APDU_HEADER_SIZE + aid.len + (le_present ? 1U : 0U))
    return 0;
  size_t pos = 0;
  dst.buf[pos++] = APDU_CLA_ISO7816;
  dst.buf[pos++] = APDU_INS_SELECT;
  dst.buf[pos++] = APDU_SELECT_AID_P2;
  dst.buf[pos++] = APDU_SELECT_FILE_P1;
  dst.buf[pos++] = (uint8_t)aid.len;
  memcpy(&dst.buf[pos], aid.ptr, aid.len);
  pos += aid.len;
  if (le_present)
    dst.buf[pos++] = le;
  return pos;
}

size_t nfc_apdu_build_select_file(nfc_apdu_buf_t dst, uint16_t fid, bool le_present, uint8_t le) {
  if (!dst.buf || dst.max < (APDU_SELECT_FILE_MIN_SIZE + (le_present ? 1U : 0U)))
    return 0;
  size_t pos = 0;
  dst.buf[pos++] = APDU_CLA_ISO7816;
  dst.buf[pos++] = APDU_INS_SELECT;
  dst.buf[pos++] = APDU_SELECT_FILE_P1;
  dst.buf[pos++] = APDU_SELECT_FILE_P2;
  dst.buf[pos++] = APDU_SELECT_FILE_LC;
  dst.buf[pos++] = (uint8_t)((fid >> APDU_BYTE_HIGH_SHIFT) & APDU_BYTE_MASK);
  dst.buf[pos++] = (uint8_t)(fid & APDU_BYTE_MASK);
  if (le_present)
    dst.buf[pos++] = le;
  return pos;
}

size_t nfc_apdu_build_read_binary(nfc_apdu_buf_t dst, uint16_t offset, uint8_t le) {
  if (!dst.buf || dst.max < APDU_HEADER_SIZE)
    return 0;
  dst.buf[0] = APDU_CLA_ISO7816;
  dst.buf[1] = APDU_INS_READ_BINARY;
  dst.buf[2] = (uint8_t)((offset >> 8) & 0xFFU);
  dst.buf[3] = (uint8_t)(offset & 0xFFU);
  dst.buf[4] = le;
  return APDU_HEADER_SIZE;
}

size_t nfc_apdu_build_update_binary(nfc_apdu_buf_t dst, uint16_t offset, nfc_apdu_data_t data) {
  if (!dst.buf || !data.ptr || data.len == 0 || data.len > APDU_LE_MAX)
    return 0;
  if (dst.max < APDU_HEADER_SIZE + data.len)
    return 0;
  size_t pos = 0;
  dst.buf[pos++] = APDU_CLA_ISO7816;
  dst.buf[pos++] = APDU_INS_UPDATE_BINARY;
  dst.buf[pos++] = (uint8_t)((offset >> 8) & 0xFFU);
  dst.buf[pos++] = (uint8_t)(offset & 0xFFU);
  dst.buf[pos++] = (uint8_t)data.len;
  memcpy(&dst.buf[pos], data.ptr, data.len);
  pos += data.len;
  return pos;
}

size_t nfc_apdu_build_verify(nfc_apdu_buf_t dst, uint8_t p1, uint8_t p2, nfc_apdu_data_t data) {
  if (!dst.buf || !data.ptr || data.len == 0 || data.len > APDU_LE_MAX)
    return 0;
  if (dst.max < APDU_HEADER_SIZE + data.len)
    return 0;
  size_t pos = 0;
  dst.buf[pos++] = APDU_CLA_ISO7816;
  dst.buf[pos++] = APDU_INS_VERIFY;
  dst.buf[pos++] = p1;
  dst.buf[pos++] = p2;
  dst.buf[pos++] = (uint8_t)data.len;
  memcpy(&dst.buf[pos], data.ptr, data.len);
  pos += data.len;
  return pos;
}

size_t nfc_apdu_build_get_data(nfc_apdu_buf_t dst, uint8_t p1, uint8_t p2, uint8_t le) {
  if (!dst.buf || dst.max < APDU_HEADER_SIZE)
    return 0;
  dst.buf[0] = APDU_CLA_ISO7816;
  dst.buf[1] = APDU_INS_GET_DATA;
  dst.buf[2] = p1;
  dst.buf[3] = p2;
  dst.buf[4] = le;
  return APDU_HEADER_SIZE;
}

bool nfc_apdu_parse_fci(const uint8_t *fci, size_t fci_len, nfc_fci_info_t *out) {
  if (!fci || fci_len < 2U || !out)
    return false;
  memset(out, 0, sizeof(*out));

  size_t pos = 0;
  while (pos + 2U <= fci_len) {
    uint8_t tag = fci[pos++];
    uint8_t len = fci[pos++];
    if (pos + len > fci_len)
      break;

    if (tag == APDU_FCI_TAG_DF_NAME) {
      size_t copy = (len > sizeof(out->df_name)) ? sizeof(out->df_name) : len;
      memcpy(out->df_name, &fci[pos], copy);
      out->df_name_len = (uint8_t)copy;
      return true;
    }

    pos += len;
  }
  return false;
}
