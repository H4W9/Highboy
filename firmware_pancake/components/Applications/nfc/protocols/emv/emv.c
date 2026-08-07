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

#include "emv.h"

#include <string.h>

#include "esp_log.h"

#include "nfc_apdu.h"

static const char *TAG = "EMV";

#define TLV_TAG_MULTI_BYTE_MASK  0x1FU /**< Low 5 bits set = multi-byte tag */
#define TLV_TAG_CONTINUATION_BIT 0x80U /**< Bit 8 set = more tag bytes follow */
#define TLV_LEN_LONG_FORM_BIT    0x80U /**< Bit 8 set = long-form length */
#define TLV_LEN_NUM_BYTES_MASK   0x7FU /**< Low 7 bits = number of length bytes */
#define TLV_CONSTRUCTED_BIT      0x20U /**< Bit 6 set = constructed (contains TLVs) */
#define TLV_TAG_MULTI_BYTE_MAX   3     /**< Max extra tag bytes before error */

#define EMV_TAG_AID          0x4FU /**< Application Identifier (AID) */
#define EMV_TAG_APP_LABEL    0x50U /**< Application Label */
#define EMV_TAG_RESP_FMT1    0x80U /**< Response Message Template Format 1 */
#define EMV_TAG_RESP_FMT2    0x77U /**< Response Message Template Format 2 */
#define EMV_TAG_CMD_TEMPLATE 0x83U /**< Command Template */
#define EMV_TAG_AFL          0x94U /**< Application File Locator */

#define EMV_CLA_PROPRIETARY   0x80U /**< CLA byte for EMV proprietary commands */
#define EMV_INS_GPO           0xA8U /**< INS byte for GET PROCESSING OPTIONS */
#define EMV_CLA_STANDARD      0x00U /**< CLA byte for standard ISO commands */
#define EMV_INS_READ_RECORD   0xB2U /**< INS byte for READ RECORD */
#define EMV_P1P2_ZERO         0x00U /**< P1/P2 zero value */
#define EMV_LE_ZERO           0x00U /**< Le = 0 (return max available) */
#define EMV_READ_REC_SFI_FLAG 0x04U /**< P2 flag: SFI-based record addressing */

#define EMV_GPO_APDU_MIN_LEN  6U    /**< Minimum GPO APDU buffer size */
#define EMV_GPO_APDU_OVERHEAD 8U    /**< GPO APDU fixed bytes (header+Lc+tag+len+Le) */
#define EMV_GPO_PDOL_MAX_LEN  0xFDU /**< Max PDOL data length */

#define EMV_AFL_ENTRY_SIZE 4U    /**< Bytes per AFL entry */
#define EMV_AFL_SFI_SHIFT  3     /**< Bits to shift for SFI extraction */
#define EMV_AFL_SFI_MASK   0x1FU /**< Mask for SFI after shift */
#define EMV_AIP_LEN        2U    /**< Application Interchange Profile length */

#define EMV_RECORD_MAX         0xFFU /**< Maximum record number */
#define EMV_RUN_BASIC_MAX_APPS 8U    /**< Max apps in emv_run_basic */
#define EMV_RUN_BASIC_MAX_AFL  12U   /**< Max AFL entries in emv_run_basic */

#define EMV_PPSE_AID_LEN 14U

#define EMV_APDU_SELECT_PPSE_SIZE 32U /**< Buffer size for PPSE select APDU */
#define EMV_APDU_SELECT_AID_SIZE  48U /**< Buffer size for AID select APDU */
#define EMV_APDU_GPO_SIZE         64U /**< Buffer size for GPO APDU */

#define EMV_RECORD_BUFFER_SIZE 256U /**< Buffer size for record read responses */
#define EMV_FCI_BUFFER_SIZE    256U /**< Buffer size for FCI responses */

#define EMV_RESP_TAG_INDEX  0U /**< Response message tag at index 0 */
#define EMV_RESP_LEN_INDEX  1U /**< Response message format 1 length at index 1 */
#define EMV_RESP_AIP_OFFSET 2U /**< Response message format 1 AIP data at offset 2 */

#define EMV_BYTE_SHIFT 8U /**< Bits to shift for multi-byte value construction */

#define EMV_TLV_LEN_1BYTE       1U /**< Single-byte length field */
#define EMV_TLV_LEN_2BYTE       2U /**< Double-byte length field */
#define EMV_TLV_LEN_2BYTE_CHECK 1U /**< Check for second byte in multi-byte length */

#define EMV_READ_REC_APDU_SIZE 5U /**< READ RECORD command APDU size */
#define EMV_READ_REC_CLA_INDEX 0U /**< CLA byte index */
#define EMV_READ_REC_INS_INDEX 1U /**< INS byte index */
#define EMV_READ_REC_P2_INDEX  3U /**< P2 byte index */
#define EMV_READ_REC_LE_INDEX  4U /**< Le byte index */

#define EMV_GPO_MIN_FMT1_LEN     2U /**< Minimum GPO response length for both formats */
#define EMV_GPO_TAG_LEN_OVERHEAD 2U /**< Tag and length bytes in GPO response */

#define EMV_AFL_MIN_BYTES        4U /**< Minimum bytes needed for AFL entry */
#define EMV_AFL_FIRST_REC_OFFSET 1U /**< First record offset within AFL entry */
#define EMV_AFL_LAST_REC_OFFSET  2U /**< Last record offset within AFL entry */
#define EMV_AFL_AUTH_REC_OFFSET  3U /**< Offline auth records offset within AFL entry */
#define EMV_AFL_ENTRY_BYTES_CHECK \
  (EMV_AFL_ENTRY_SIZE - 1U) /**< Max offset before reading next entry */

#define EMV_APP_FIRST_INDEX 0U /**< Index of first application in apps array */

static const uint8_t k_ppse_aid[EMV_PPSE_AID_LEN] = {
    '2', 'P', 'A', 'Y', '.', 'S', 'Y', 'S', '.', 'D', 'D', 'F', '0', '1'};

typedef struct {
  uint32_t tag;
  size_t len;
  const uint8_t *value;
  size_t tlv_len;
  bool constructed;
} emv_tlv_t;

static bool emv_tlv_next(const uint8_t *data, size_t len, size_t *pos, emv_tlv_t *out) {
  if (data == NULL || pos == NULL || out == NULL)
    return false;
  if (*pos >= len)
    return false;

  size_t start = *pos;
  size_t p = *pos;
  uint8_t first = data[p++];
  uint32_t tag = first;
  if ((first & TLV_TAG_MULTI_BYTE_MASK) == TLV_TAG_MULTI_BYTE_MASK) {
    int cnt = 0;
    uint8_t b = 0;
    do {
      if (p >= len)
        return false;
      b = data[p++];
      tag = (tag << 8) | b;
      cnt++;
    } while ((b & TLV_TAG_CONTINUATION_BIT) && cnt < TLV_TAG_MULTI_BYTE_MAX);
    if (cnt >= TLV_TAG_MULTI_BYTE_MAX && (b & TLV_TAG_CONTINUATION_BIT))
      return false;
  }

  if (p >= len)
    return false;
  uint8_t l = data[p++];
  size_t vlen = 0;
  if ((l & TLV_LEN_LONG_FORM_BIT) == 0) {
    vlen = l;
  } else {
    uint8_t n = (uint8_t)(l & TLV_LEN_NUM_BYTES_MASK);
    if (n == EMV_TLV_LEN_1BYTE) {
      if (p >= len)
        return false;
      vlen = data[p++];
    } else if (n == EMV_TLV_LEN_2BYTE) {
      if (p + EMV_TLV_LEN_2BYTE_CHECK >= len)
        return false;
      vlen = ((size_t)data[p] << EMV_BYTE_SHIFT) | data[p + 1];
      p += EMV_TLV_LEN_2BYTE;
    } else {
      return false;
    }
  }

  if (p + vlen > len)
    return false;

  out->tag = tag;
  out->len = vlen;
  out->value = &data[p];
  out->tlv_len = (p - start) + vlen;
  out->constructed = (first & TLV_CONSTRUCTED_BIT) != 0;
  *pos = p + vlen;
  return true;
}

static bool
emv_tlv_find_tag(const uint8_t *data, size_t len, uint32_t tag, const uint8_t **val, size_t *vlen) {
  size_t pos = 0;
  emv_tlv_t tlv;
  while (emv_tlv_next(data, len, &pos, &tlv)) {
    if (tlv.tag == tag) {
      if (val)
        *val = tlv.value;
      if (vlen)
        *vlen = tlv.len;
      return true;
    }
    if (tlv.constructed && tlv.len > 0) {
      if (emv_tlv_find_tag(tlv.value, tlv.len, tag, val, vlen))
        return true;
    }
  }
  return false;
}

typedef struct {
  emv_app_t *apps;
  size_t max;
  size_t count;
  emv_app_t *last;
} emv_app_list_t;

static void emv_extract_cb(const emv_tlv_t *tlv, void *user) {
  if (tlv == NULL || user == NULL)
    return;
  emv_app_list_t *list = (emv_app_list_t *)user;
  if (tlv->tag == EMV_TAG_AID && tlv->len > 0) {
    if (list->count >= list->max)
      return;
    emv_app_t *app = &list->apps[list->count++];
    memset(app, 0, sizeof(*app));
    size_t copy = (tlv->len > EMV_AID_MAX_LEN) ? EMV_AID_MAX_LEN : tlv->len;
    memcpy(app->aid, tlv->value, copy);
    app->aid_len = (uint8_t)copy;
    list->last = app;
    return;
  }
  if (tlv->tag == EMV_TAG_APP_LABEL && tlv->len > 0 && list->last) {
    size_t copy = (tlv->len > EMV_APP_LABEL_MAX) ? EMV_APP_LABEL_MAX : tlv->len;
    memcpy(list->last->label, tlv->value, copy);
    list->last->label[copy] = '\0';
  }
}

static void
emv_tlv_scan(const uint8_t *data, size_t len, void (*cb)(const emv_tlv_t *, void *), void *user) {
  size_t pos = 0;
  emv_tlv_t tlv;
  while (emv_tlv_next(data, len, &pos, &tlv)) {
    if (cb)
      cb(&tlv, user);
    if (tlv.constructed && tlv.len > 0) {
      emv_tlv_scan(tlv.value, tlv.len, cb, user);
    }
  }
}

hb_nfc_err_t emv_select_ppse(hb_nfc_protocol_t proto,
                             const void *ctx,
                             uint8_t *fci,
                             size_t fci_max,
                             size_t *fci_len,
                             uint16_t *sw,
                             int timeout_ms) {
  uint8_t apdu[EMV_APDU_SELECT_PPSE_SIZE];
  size_t apdu_len = nfc_apdu_build_select_aid((nfc_apdu_buf_t){apdu, sizeof(apdu)},
                                              (nfc_apdu_data_t){k_ppse_aid, EMV_PPSE_AID_LEN},
                                              true,
                                              EMV_P1P2_ZERO);
  if (apdu_len == 0)
    return HB_NFC_ERR_PARAM;
  nfc_apdu_channel_t ch = {.proto = proto, .ctx = ctx, .timeout_ms = timeout_ms};
  nfc_apdu_resp_t resp = {.buf = fci, .max = fci_max};
  hb_nfc_err_t err = nfc_apdu_transceive(&ch, apdu, apdu_len, &resp);
  *fci_len = resp.len;
  *sw = resp.sw;
  return err;
}

size_t emv_extract_aids(const uint8_t *fci, size_t fci_len, emv_app_t *out, size_t max) {
  if (fci == NULL || out == NULL || max == 0)
    return 0;
  emv_app_list_t list = {.apps = out, .max = max, .count = 0, .last = NULL};
  emv_tlv_scan(fci, fci_len, emv_extract_cb, &list);
  return list.count;
}

hb_nfc_err_t emv_select_aid(hb_nfc_protocol_t proto,
                            const void *ctx,
                            const uint8_t *aid,
                            size_t aid_len,
                            uint8_t *fci,
                            size_t fci_max,
                            size_t *fci_len,
                            uint16_t *sw,
                            int timeout_ms) {
  uint8_t apdu[EMV_APDU_SELECT_AID_SIZE];
  size_t apdu_len = nfc_apdu_build_select_aid(
      (nfc_apdu_buf_t){apdu, sizeof(apdu)}, (nfc_apdu_data_t){aid, aid_len}, true, EMV_P1P2_ZERO);
  if (apdu_len == 0)
    return HB_NFC_ERR_PARAM;
  nfc_apdu_channel_t ch = {.proto = proto, .ctx = ctx, .timeout_ms = timeout_ms};
  nfc_apdu_resp_t resp = {.buf = fci, .max = fci_max};
  hb_nfc_err_t err = nfc_apdu_transceive(&ch, apdu, apdu_len, &resp);
  *fci_len = resp.len;
  *sw = resp.sw;
  return err;
}

static size_t emv_build_gpo_apdu(uint8_t *out, size_t max, const uint8_t *pdol, size_t pdol_len) {
  if (out == NULL || max < EMV_GPO_APDU_MIN_LEN)
    return 0;
  if (pdol_len > EMV_GPO_PDOL_MAX_LEN)
    return 0;
  size_t lc = 2U + pdol_len;
  if (max < (EMV_GPO_APDU_OVERHEAD + pdol_len))
    return 0;

  size_t pos = 0;
  out[pos++] = EMV_CLA_PROPRIETARY;
  out[pos++] = EMV_INS_GPO;
  out[pos++] = EMV_P1P2_ZERO;
  out[pos++] = EMV_P1P2_ZERO;
  out[pos++] = (uint8_t)lc;
  out[pos++] = EMV_TAG_CMD_TEMPLATE;
  out[pos++] = (uint8_t)pdol_len;
  if (pdol_len > 0 && pdol) {
    memcpy(&out[pos], pdol, pdol_len);
    pos += pdol_len;
  }
  out[pos++] = EMV_LE_ZERO; /* Le */
  return pos;
}

hb_nfc_err_t emv_get_processing_options(hb_nfc_protocol_t proto,
                                        const void *ctx,
                                        const uint8_t *pdol,
                                        size_t pdol_len,
                                        uint8_t *gpo,
                                        size_t gpo_max,
                                        size_t *gpo_len,
                                        uint16_t *sw,
                                        int timeout_ms) {
  uint8_t apdu[EMV_APDU_GPO_SIZE];
  size_t apdu_len = emv_build_gpo_apdu(apdu, sizeof(apdu), pdol, pdol_len);
  if (apdu_len == 0)
    return HB_NFC_ERR_PARAM;
  nfc_apdu_channel_t ch = {.proto = proto, .ctx = ctx, .timeout_ms = timeout_ms};
  nfc_apdu_resp_t resp = {.buf = gpo, .max = gpo_max};
  hb_nfc_err_t err = nfc_apdu_transceive(&ch, apdu, apdu_len, &resp);
  *gpo_len = resp.len;
  *sw = resp.sw;
  return err;
}

size_t emv_parse_afl(const uint8_t *gpo, size_t gpo_len, emv_afl_entry_t *out, size_t max) {
  if (gpo == NULL || gpo_len < 2U || out == NULL || max == 0)
    return 0;

  const uint8_t *afl = NULL;
  size_t afl_len = 0;

  if (gpo[EMV_RESP_TAG_INDEX] == EMV_TAG_RESP_FMT1) {
    size_t val_len = gpo[EMV_RESP_LEN_INDEX];
    if (val_len + 2U > gpo_len || val_len < EMV_AIP_LEN)
      return 0;
    afl = &gpo[EMV_RESP_AIP_OFFSET + EMV_AIP_LEN];
    afl_len = val_len - EMV_AIP_LEN;
  } else if (gpo[EMV_RESP_TAG_INDEX] == EMV_TAG_RESP_FMT2) {
    if (!emv_tlv_find_tag(gpo, gpo_len, EMV_TAG_AFL, &afl, &afl_len))
      return 0;
  } else {
    return 0;
  }

  size_t count = 0;
  for (size_t i = 0; i + 3U < afl_len && count < max; i += EMV_AFL_ENTRY_SIZE) {
    emv_afl_entry_t *e = &out[count++];
    e->sfi = (uint8_t)((afl[i] >> EMV_AFL_SFI_SHIFT) & EMV_AFL_SFI_MASK);
    e->record_first = afl[i + 1];
    e->record_last = afl[i + 2];
    e->offline_auth_records = afl[i + 3];
  }
  return count;
}

hb_nfc_err_t emv_read_record(hb_nfc_protocol_t proto,
                             const void *ctx,
                             uint8_t sfi,
                             uint8_t record,
                             uint8_t *out,
                             size_t out_max,
                             size_t *out_len,
                             uint16_t *sw,
                             int timeout_ms) {
  if (out == NULL || out_max < 2U)
    return HB_NFC_ERR_PARAM;
  uint8_t apdu[5];
  apdu[0] = EMV_CLA_STANDARD;
  apdu[1] = EMV_INS_READ_RECORD;
  apdu[2] = record;
  apdu[3] = (uint8_t)((sfi << EMV_AFL_SFI_SHIFT) | EMV_READ_REC_SFI_FLAG);
  apdu[4] = EMV_LE_ZERO; /* Le */
  nfc_apdu_channel_t ch = {.proto = proto, .ctx = ctx, .timeout_ms = timeout_ms};
  nfc_apdu_resp_t resp = {.buf = out, .max = out_max};
  hb_nfc_err_t err = nfc_apdu_transceive(&ch, apdu, sizeof(apdu), &resp);
  *out_len = resp.len;
  *sw = resp.sw;
  return err;
}

hb_nfc_err_t emv_read_records(hb_nfc_protocol_t proto,
                              const void *ctx,
                              const emv_afl_entry_t *afl,
                              size_t afl_count,
                              emv_record_cb_t cb,
                              void *user,
                              int timeout_ms) {
  if (afl == NULL || afl_count == 0)
    return HB_NFC_ERR_PARAM;

  uint8_t resp[EMV_RECORD_BUFFER_SIZE];
  for (size_t i = 0; i < afl_count; i++) {
    const emv_afl_entry_t *e = &afl[i];
    for (uint8_t rec = e->record_first; rec <= e->record_last; rec++) {
      size_t rlen = 0;
      uint16_t sw = 0;
      hb_nfc_err_t err =
          emv_read_record(proto, ctx, e->sfi, rec, resp, sizeof(resp), &rlen, &sw, timeout_ms);
      if (err != HB_NFC_OK)
        return err;
      if (cb)
        cb(e->sfi, rec, resp, rlen, user);
      if (rec == EMV_RECORD_MAX)
        break;
    }
  }
  return HB_NFC_OK;
}

hb_nfc_err_t emv_run_basic(hb_nfc_protocol_t proto,
                           const void *ctx,
                           emv_app_t *app_out,
                           emv_record_cb_t cb,
                           void *user,
                           int timeout_ms) {
  uint8_t fci[EMV_FCI_BUFFER_SIZE];
  size_t fci_len = 0;
  uint16_t sw = 0;
  hb_nfc_err_t err = emv_select_ppse(proto, ctx, fci, sizeof(fci), &fci_len, &sw, timeout_ms);
  if (err != HB_NFC_OK)
    return err;

  emv_app_t apps[EMV_RUN_BASIC_MAX_APPS];
  size_t app_count = emv_extract_aids(fci, fci_len, apps, EMV_RUN_BASIC_MAX_APPS);
  if (app_count == 0)
    return HB_NFC_ERR_PROTOCOL;

  emv_app_t *app = &apps[0];
  if (app_out)
    *app_out = *app;

  uint8_t fci_app[EMV_FCI_BUFFER_SIZE];
  size_t fci_app_len = 0;
  err = emv_select_aid(
      proto, ctx, app->aid, app->aid_len, fci_app, sizeof(fci_app), &fci_app_len, &sw, timeout_ms);
  if (err != HB_NFC_OK)
    return err;

  uint8_t gpo[EMV_FCI_BUFFER_SIZE];
  size_t gpo_len = 0;
  err =
      emv_get_processing_options(proto, ctx, NULL, 0, gpo, sizeof(gpo), &gpo_len, &sw, timeout_ms);
  if (err != HB_NFC_OK)
    return err;

  emv_afl_entry_t afl[EMV_RUN_BASIC_MAX_AFL];
  size_t afl_count = emv_parse_afl(gpo, gpo_len, afl, EMV_RUN_BASIC_MAX_AFL);
  if (afl_count == 0)
    return HB_NFC_ERR_PROTOCOL;

  return emv_read_records(proto, ctx, afl, afl_count, cb, user, timeout_ms);
}
