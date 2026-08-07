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
 * @file ndef.c
 * @brief NDEF record parser, builder, and TLV helpers.
 * Reference: NFC Forum NDEF 1.0, NFC Forum Type 2/4 Tag spec.
 */
#include "ndef.h"

#include <string.h>

#include "esp_log.h"

#define NDEF_HDR_MB       0x80U /**< Message Begin */
#define NDEF_HDR_ME       0x40U /**< Message End */
#define NDEF_HDR_CF       0x20U /**< Chunk Flag */
#define NDEF_HDR_SR       0x10U /**< Short Record */
#define NDEF_HDR_IL       0x08U /**< ID Length present */
#define NDEF_HDR_TNF_MASK 0x07U /**< Type Name Format mask */

#define NDEF_TEXT_UTF16_BIT 0x80U /**< UTF-16 indicator in status byte */
#define NDEF_TEXT_LANG_MASK 0x3FU /**< Language code length mask */

#define NDEF_BYTE_MASK       0xFFU   /**< Single-byte mask / TLV 3-byte marker */
#define NDEF_MAX_LANG_LEN    63U     /**< Maximum language code length */
#define NDEF_TLV_LEN3_MARKER 0xFFU   /**< TLV three-byte length indicator */
#define NDEF_TLV_MAX_VALUE   0xFFFFU /**< Maximum TLV value length */
#define NDEF_LONG_PL_BYTES   4U      /**< Bytes in a long payload-length field */
#define NDEF_PAYLOAD_SHIFT24 24U
#define NDEF_PAYLOAD_SHIFT16 16U
#define NDEF_PAYLOAD_SHIFT8  8U
#define NDEF_MIN_MSG_LEN     3U   /**< Minimum bytes for a valid NDEF message */
#define NDEF_TLV_LEN3_EXTRA  2U   /**< Extra bytes after 0xFF marker in 3-byte TLV length */
#define NDEF_TLV_LEN3_SIZE   3U   /**< Total bytes for a 3-byte TLV length field */
#define NDEF_LOG_BUF_SIZE    96U  /**< Buffer size for log_str helper */
#define NDEF_URI_BUF_SIZE    128U /**< Buffer size for URI decode in ndef_print */

static const char *TAG = "NFC_NDEF";

static const char *uri_prefix(uint8_t code);
static uint8_t uri_prefix_match(const char *uri, size_t uri_len, size_t *prefix_len);
static void log_str(const char *label, const uint8_t *data, size_t len);
static hb_nfc_err_t ndef_builder_begin_record(ndef_builder_t *b,
                                              uint8_t tnf,
                                              const uint8_t *type,
                                              uint8_t type_len,
                                              const uint8_t *id,
                                              uint8_t id_len,
                                              uint32_t payload_len,
                                              uint8_t **payload_out);

static const char *k_uri_prefixes[] = {"",
                                       "http://www.",
                                       "https://www.",
                                       "http://",
                                       "https://",
                                       "tel:",
                                       "mailto:",
                                       "ftp://anonymous:anonymous@",
                                       "ftp://ftp.",
                                       "ftps://",
                                       "sftp://",
                                       "smb://",
                                       "nfs://",
                                       "ftp://",
                                       "dav://",
                                       "news:",
                                       "telnet://",
                                       "imap:",
                                       "rtsp://",
                                       "urn:",
                                       "pop:",
                                       "sip:",
                                       "sips:",
                                       "tftp:",
                                       "btspp://",
                                       "btl2cap://",
                                       "btgoep://",
                                       "tcpobex://",
                                       "irdaobex://",
                                       "file://",
                                       "urn:epc:id:",
                                       "urn:epc:tag:",
                                       "urn:epc:pat:",
                                       "urn:epc:raw:",
                                       "urn:epc:",
                                       "urn:nfc:"};

static const char *uri_prefix(uint8_t code) {
  if (code < (sizeof(k_uri_prefixes) / sizeof(k_uri_prefixes[0]))) {
    return k_uri_prefixes[code];
  }
  return "";
}

static uint8_t uri_prefix_match(const char *uri, size_t uri_len, size_t *prefix_len) {
  uint8_t best = 0;
  size_t best_len = 0;
  if (uri == NULL) {
    if (prefix_len != NULL)
      *prefix_len = 0;
    return 0;
  }

  uint8_t count = (uint8_t)(sizeof(k_uri_prefixes) / sizeof(k_uri_prefixes[0]));
  for (uint8_t i = 1; i < count; i++) {
    const char *p = k_uri_prefixes[i];
    if (p == NULL || p[0] == '\0')
      continue;
    size_t l = strlen(p);
    if (l == 0 || l > uri_len)
      continue;
    if (strncmp(uri, p, l) == 0 && l > best_len) {
      best_len = l;
      best = i;
    }
    if (l == uri_len && best_len == uri_len)
      break;
  }

  if (prefix_len != NULL)
    *prefix_len = best_len;
  return best;
}

static void log_str(const char *label, const uint8_t *data, size_t len) {
  char buf[NDEF_LOG_BUF_SIZE];
  size_t n = (len < (sizeof(buf) - 1)) ? len : (sizeof(buf) - 1);
  memcpy(buf, data, n);
  buf[n] = '\0';
  ESP_LOGI(TAG, "%s %s", label, buf);
}

void ndef_parser_init(ndef_parser_t *parser, const uint8_t *data, size_t len) {
  if (parser == NULL)
    return;
  parser->data = data;
  parser->len = len;
  parser->pos = 0;
  parser->done = false;
  parser->err = HB_NFC_OK;
}

bool ndef_parse_next(ndef_parser_t *parser, ndef_record_t *rec) {
  if (parser == NULL || rec == NULL || parser->done)
    return false;
  if (parser->data == NULL || parser->len < NDEF_MIN_MSG_LEN || parser->pos >= parser->len) {
    parser->done = true;
    parser->err = HB_NFC_OK;
    return false;
  }

  const uint8_t *data = parser->data;
  size_t len = parser->len;
  size_t pos = parser->pos;

  if (pos >= len) {
    parser->done = true;
    parser->err = HB_NFC_OK;
    return false;
  }

  uint8_t hdr = data[pos++];
  rec->mb = (hdr & NDEF_HDR_MB) != 0;
  rec->me = (hdr & NDEF_HDR_ME) != 0;
  rec->cf = (hdr & NDEF_HDR_CF) != 0;
  rec->sr = (hdr & NDEF_HDR_SR) != 0;
  rec->il = (hdr & NDEF_HDR_IL) != 0;
  rec->tnf = (uint8_t)(hdr & NDEF_HDR_TNF_MASK);

  if (rec->cf) {
    parser->err = HB_NFC_ERR_PROTOCOL;
    parser->done = true;
    return false;
  }

  if (pos >= len) {
    parser->err = HB_NFC_ERR_PROTOCOL;
    parser->done = true;
    return false;
  }
  rec->type_len = data[pos++];

  if (rec->sr) {
    if (pos >= len) {
      parser->err = HB_NFC_ERR_PROTOCOL;
      parser->done = true;
      return false;
    }
    rec->payload_len = data[pos++];
  } else {
    if (pos + NDEF_LONG_PL_BYTES > len) {
      parser->err = HB_NFC_ERR_PROTOCOL;
      parser->done = true;
      return false;
    }
    rec->payload_len = ((uint32_t)data[pos] << NDEF_PAYLOAD_SHIFT24) |
                       ((uint32_t)data[pos + 1] << NDEF_PAYLOAD_SHIFT16) |
                       ((uint32_t)data[pos + 2] << NDEF_PAYLOAD_SHIFT8) | (uint32_t)data[pos + 3];
    pos += NDEF_LONG_PL_BYTES;
  }

  rec->id_len = 0;
  if (rec->il) {
    if (pos >= len) {
      parser->err = HB_NFC_ERR_PROTOCOL;
      parser->done = true;
      return false;
    }
    rec->id_len = data[pos++];
  }

  size_t needed = (size_t)rec->type_len + (size_t)rec->id_len + (size_t)rec->payload_len;
  if (pos + needed > len) {
    parser->err = HB_NFC_ERR_PROTOCOL;
    parser->done = true;
    return false;
  }

  rec->type = &data[pos];
  pos += rec->type_len;
  rec->id = &data[pos];
  pos += rec->id_len;
  rec->payload = &data[pos];
  pos += rec->payload_len;

  parser->pos = pos;
  if (rec->me || parser->pos >= parser->len)
    parser->done = true;
  parser->err = HB_NFC_OK;
  return true;
}

hb_nfc_err_t ndef_parser_error(const ndef_parser_t *parser) {
  if (parser == NULL)
    return HB_NFC_ERR_PARAM;
  return parser->err;
}

bool ndef_record_is_text(const ndef_record_t *rec) {
  return rec && rec->tnf == NDEF_TNF_WELL_KNOWN && rec->type_len == 1 && rec->type &&
         rec->type[0] == 'T';
}

bool ndef_record_is_uri(const ndef_record_t *rec) {
  return rec && rec->tnf == NDEF_TNF_WELL_KNOWN && rec->type_len == 1 && rec->type &&
         rec->type[0] == 'U';
}

bool ndef_record_is_smart_poster(const ndef_record_t *rec) {
  return rec && rec->tnf == NDEF_TNF_WELL_KNOWN && rec->type_len == 2 && rec->type &&
         rec->type[0] == 'S' && rec->type[1] == 'p';
}

bool ndef_record_is_mime(const ndef_record_t *rec) {
  return rec && rec->tnf == NDEF_TNF_MIME && rec->type_len > 0 && rec->type;
}

bool ndef_decode_text(const ndef_record_t *rec, ndef_text_t *out) {
  if (rec == NULL || out == NULL || !ndef_record_is_text(rec))
    return false;
  if (rec->payload_len < 1U)
    return false;
  uint8_t status = rec->payload[0];
  uint8_t lang_len = (uint8_t)(status & NDEF_TEXT_LANG_MASK);
  if (1U + lang_len > rec->payload_len)
    return false;
  out->utf16 = (status & NDEF_TEXT_UTF16_BIT) != 0;
  out->lang = (lang_len > 0) ? (const uint8_t *)(rec->payload + 1) : NULL;
  out->lang_len = lang_len;
  out->text = rec->payload + 1U + lang_len;
  out->text_len = (size_t)rec->payload_len - 1U - lang_len;
  return true;
}

bool ndef_decode_uri(const ndef_record_t *rec, char *out, size_t out_max, size_t *out_len) {
  if (rec == NULL || !ndef_record_is_uri(rec) || rec->payload_len < 1U)
    return false;
  uint8_t prefix_code = rec->payload[0];
  const char *prefix = uri_prefix(prefix_code);
  size_t prefix_len = strlen(prefix);
  size_t rest_len = (size_t)rec->payload_len - 1U;
  size_t total = prefix_len + rest_len;
  if (out_len != NULL)
    *out_len = total;
  if (out == NULL || out_max == 0)
    return true;
  if (out_max < total + 1U)
    return false;
  if (prefix_len > 0)
    memcpy(out, prefix, prefix_len);
  if (rest_len > 0)
    memcpy(out + prefix_len, rec->payload + 1, rest_len);
  out[total] = '\0';
  return true;
}

void ndef_builder_init(ndef_builder_t *b, uint8_t *out, size_t max) {
  if (b == NULL)
    return;
  b->buf = out;
  b->max = max;
  b->len = 0;
  b->last_hdr_off = 0;
  b->has_record = false;
}

size_t ndef_builder_len(const ndef_builder_t *b) {
  return b ? b->len : 0;
}

static hb_nfc_err_t ndef_builder_begin_record(ndef_builder_t *b,
                                              uint8_t tnf,
                                              const uint8_t *type,
                                              uint8_t type_len,
                                              const uint8_t *id,
                                              uint8_t id_len,
                                              uint32_t payload_len,
                                              uint8_t **payload_out) {
  if (b == NULL || b->buf == NULL)
    return HB_NFC_ERR_PARAM;
  if (payload_len > 0 && payload_out == NULL)
    return HB_NFC_ERR_PARAM;
  if (type_len > 0 && type == NULL)
    return HB_NFC_ERR_PARAM;
  if (id_len > 0 && id == NULL)
    return HB_NFC_ERR_PARAM;

  bool sr = payload_len <= NDEF_BYTE_MASK;
  bool il = (id != NULL) && (id_len > 0);
  size_t needed = 1U + 1U + (sr ? 1U : NDEF_LONG_PL_BYTES) + (il ? 1U : 0U) + type_len + id_len +
                  (size_t)payload_len;
  if (b->len + needed > b->max)
    return HB_NFC_ERR_PARAM;

  if (b->has_record) {
    b->buf[b->last_hdr_off] &= (uint8_t)~NDEF_HDR_ME;
  }

  size_t hdr_off = b->len;
  uint8_t hdr = (uint8_t)(tnf & NDEF_HDR_TNF_MASK);
  if (!b->has_record)
    hdr |= NDEF_HDR_MB;
  hdr |= NDEF_HDR_ME;
  if (sr)
    hdr |= NDEF_HDR_SR;
  if (il)
    hdr |= NDEF_HDR_IL;

  b->buf[b->len++] = hdr;
  b->buf[b->len++] = type_len;
  if (sr) {
    b->buf[b->len++] = (uint8_t)payload_len;
  } else {
    b->buf[b->len++] = (uint8_t)((payload_len >> NDEF_PAYLOAD_SHIFT24) & NDEF_BYTE_MASK);
    b->buf[b->len++] = (uint8_t)((payload_len >> NDEF_PAYLOAD_SHIFT16) & NDEF_BYTE_MASK);
    b->buf[b->len++] = (uint8_t)((payload_len >> NDEF_PAYLOAD_SHIFT8) & NDEF_BYTE_MASK);
    b->buf[b->len++] = (uint8_t)(payload_len & NDEF_BYTE_MASK);
  }
  if (il)
    b->buf[b->len++] = id_len;

  if (type_len > 0) {
    memcpy(&b->buf[b->len], type, type_len);
    b->len += type_len;
  }
  if (id_len > 0) {
    memcpy(&b->buf[b->len], id, id_len);
    b->len += id_len;
  }

  if (payload_out != NULL)
    *payload_out = &b->buf[b->len];
  b->len += payload_len;
  b->last_hdr_off = hdr_off;
  b->has_record = true;
  return HB_NFC_OK;
}

hb_nfc_err_t ndef_builder_add_record(ndef_builder_t *b, const ndef_record_data_t *rec) {
  if (rec == NULL)
    return HB_NFC_ERR_PARAM;
  if (rec->payload_len > 0 && rec->payload == NULL)
    return HB_NFC_ERR_PARAM;
  uint8_t *dst = NULL;
  hb_nfc_err_t err = ndef_builder_begin_record(
      b, rec->tnf, rec->type, rec->type_len, rec->id, rec->id_len, rec->payload_len, &dst);
  if (err != HB_NFC_OK)
    return err;
  if (rec->payload_len > 0 && rec->payload != NULL)
    memcpy(dst, rec->payload, rec->payload_len);
  return HB_NFC_OK;
}

hb_nfc_err_t
ndef_builder_add_text(ndef_builder_t *b, const char *text, const char *lang, bool utf16) {
  if (b == NULL || text == NULL)
    return HB_NFC_ERR_PARAM;
  size_t text_len = strlen(text);
  size_t lang_len = (lang != NULL) ? strlen(lang) : 0;
  if (lang_len > NDEF_MAX_LANG_LEN)
    return HB_NFC_ERR_PARAM;

  uint32_t payload_len = (uint32_t)(1U + lang_len + text_len);
  uint8_t *payload = NULL;
  hb_nfc_err_t err = ndef_builder_begin_record(
      b, NDEF_TNF_WELL_KNOWN, (const uint8_t *)"T", 1, NULL, 0, payload_len, &payload);
  if (err != HB_NFC_OK)
    return err;

  payload[0] = (uint8_t)((utf16 ? NDEF_TEXT_UTF16_BIT : 0U) | (uint8_t)lang_len);
  if (lang_len > 0)
    memcpy(&payload[1], lang, lang_len);
  if (text_len > 0)
    memcpy(&payload[1 + lang_len], text, text_len);
  return HB_NFC_OK;
}

hb_nfc_err_t ndef_builder_add_uri(ndef_builder_t *b, const char *uri) {
  if (b == NULL || uri == NULL)
    return HB_NFC_ERR_PARAM;
  size_t uri_len = strlen(uri);
  size_t prefix_len = 0;
  uint8_t prefix_code = uri_prefix_match(uri, uri_len, &prefix_len);

  uint32_t payload_len = (uint32_t)(1U + (uri_len - prefix_len));
  uint8_t *payload = NULL;
  hb_nfc_err_t err = ndef_builder_begin_record(
      b, NDEF_TNF_WELL_KNOWN, (const uint8_t *)"U", 1, NULL, 0, payload_len, &payload);
  if (err != HB_NFC_OK)
    return err;

  payload[0] = prefix_code;
  if (uri_len > prefix_len) {
    memcpy(&payload[1], uri + prefix_len, uri_len - prefix_len);
  }
  return HB_NFC_OK;
}

hb_nfc_err_t ndef_builder_add_mime(ndef_builder_t *b,
                                   const char *mime_type,
                                   const uint8_t *data,
                                   size_t data_len) {
  if (b == NULL || mime_type == NULL)
    return HB_NFC_ERR_PARAM;
  size_t type_len = strlen(mime_type);
  if (type_len == 0 || type_len > NDEF_BYTE_MASK)
    return HB_NFC_ERR_PARAM;
  const ndef_record_data_t rec = {
      .tnf = NDEF_TNF_MIME,
      .type = (const uint8_t *)mime_type,
      .type_len = (uint8_t)type_len,
      .id = NULL,
      .id_len = 0,
      .payload = data,
      .payload_len = (uint32_t)data_len,
  };
  return ndef_builder_add_record(b, &rec);
}

hb_nfc_err_t
ndef_builder_add_smart_poster(ndef_builder_t *b, const uint8_t *nested_ndef, size_t nested_len) {
  if (b == NULL || nested_ndef == NULL || nested_len == 0)
    return HB_NFC_ERR_PARAM;
  const ndef_record_data_t rec = {
      .tnf = NDEF_TNF_WELL_KNOWN,
      .type = (const uint8_t *)"Sp",
      .type_len = 2,
      .id = NULL,
      .id_len = 0,
      .payload = nested_ndef,
      .payload_len = (uint32_t)nested_len,
  };
  return ndef_builder_add_record(b, &rec);
}

ndef_tlv_status_t ndef_tlv_find(const uint8_t *data, size_t len, uint8_t type, ndef_tlv_t *out) {
  if (data == NULL || len == 0 || out == NULL)
    return NDEF_TLV_STATUS_INVALID;
  size_t pos = 0;
  while (pos < len) {
    size_t start = pos;
    uint8_t t = data[pos++];
    if (t == NDEF_TLV_NULL)
      continue;
    if (t == NDEF_TLV_TERMINATOR)
      return NDEF_TLV_STATUS_NOT_FOUND;
    if (pos >= len)
      return NDEF_TLV_STATUS_INVALID;

    size_t l = 0;
    if (data[pos] == NDEF_TLV_LEN3_MARKER) {
      if (pos + NDEF_TLV_LEN3_EXTRA >= len)
        return NDEF_TLV_STATUS_INVALID;
      l = ((size_t)data[pos + 1] << NDEF_PAYLOAD_SHIFT8) | data[pos + 2];
      pos += NDEF_TLV_LEN3_SIZE;
    } else {
      l = data[pos++];
    }

    if (pos + l > len)
      return NDEF_TLV_STATUS_INVALID;
    if (t == type) {
      out->type = t;
      out->length = l;
      out->value = &data[pos];
      out->tlv_len = (pos - start) + l;
      return NDEF_TLV_STATUS_OK;
    }
    pos += l;
  }
  return NDEF_TLV_STATUS_NOT_FOUND;
}

ndef_tlv_status_t ndef_tlv_find_ndef(const uint8_t *data, size_t len, ndef_tlv_t *out) {
  return ndef_tlv_find(data, len, NDEF_TLV_NDEF, out);
}

ndef_tlv_status_t ndef_tlv_build(uint8_t *out, size_t max, const ndef_tlv_t *tlv, size_t *out_len) {
  if (out == NULL || max == 0 || tlv == NULL)
    return NDEF_TLV_STATUS_INVALID;
  size_t value_len = tlv->length;
  if (value_len > NDEF_TLV_MAX_VALUE)
    return NDEF_TLV_STATUS_INVALID;

  size_t len_field = (value_len < NDEF_BYTE_MASK) ? 1U : NDEF_TLV_LEN3_SIZE;
  size_t needed = 1U + len_field + value_len;
  if (needed > max)
    return NDEF_TLV_STATUS_INVALID;

  size_t pos = 0;
  out[pos++] = tlv->type;
  if (value_len < NDEF_BYTE_MASK) {
    out[pos++] = (uint8_t)value_len;
  } else {
    out[pos++] = NDEF_TLV_LEN3_MARKER;
    out[pos++] = (uint8_t)((value_len >> NDEF_PAYLOAD_SHIFT8) & NDEF_BYTE_MASK);
    out[pos++] = (uint8_t)(value_len & NDEF_BYTE_MASK);
  }
  if (value_len > 0 && tlv->value != NULL) {
    memcpy(&out[pos], tlv->value, value_len);
  }
  pos += value_len;
  if (out_len != NULL)
    *out_len = pos;
  return NDEF_TLV_STATUS_OK;
}

ndef_tlv_status_t
ndef_tlv_build_ndef(uint8_t *out, size_t max, const ndef_tlv_t *tlv, size_t *out_len) {
  if (out == NULL || max == 0 || tlv == NULL)
    return NDEF_TLV_STATUS_INVALID;
  size_t tlv_len = 0;
  ndef_tlv_status_t st = ndef_tlv_build(out, max, tlv, &tlv_len);
  if (st != NDEF_TLV_STATUS_OK)
    return st;
  if (tlv_len + 1U > max)
    return NDEF_TLV_STATUS_INVALID;
  out[tlv_len++] = NDEF_TLV_TERMINATOR;
  if (out_len != NULL)
    *out_len = tlv_len;
  return NDEF_TLV_STATUS_OK;
}

void ndef_print(const uint8_t *data, size_t len) {
  if (data == NULL || len < NDEF_MIN_MSG_LEN) {
    ESP_LOGW(TAG, "NDEF empty/invalid");
    return;
  }

  ndef_parser_t parser;
  ndef_parser_init(&parser, data, len);
  ndef_record_t rec;
  int idx = 0;

  while (ndef_parse_next(&parser, &rec)) {
    ESP_LOGI(TAG, "Record %d: MB=%d ME=%d SR=%d TNF=0x%02X", idx, rec.mb, rec.me, rec.sr, rec.tnf);

    if (ndef_record_is_text(&rec)) {
      ndef_text_t text;
      if (ndef_decode_text(&rec, &text)) {
        ESP_LOGI(TAG, "NDEF Text (%s):", text.utf16 ? "UTF-16" : "UTF-8");
        if (text.lang_len > 0 && text.lang != NULL) {
          log_str("Lang:", text.lang, text.lang_len);
        }
        if (text.text_len > 0 && text.text != NULL) {
          log_str("Text:", text.text, text.text_len);
        }
      }
    } else if (ndef_record_is_uri(&rec)) {
      char buf[NDEF_URI_BUF_SIZE];
      size_t out_len = 0;
      if (ndef_decode_uri(&rec, buf, sizeof(buf), &out_len)) {
        ESP_LOGI(TAG, "NDEF URI (%u bytes): %s", (unsigned)out_len, buf);
      } else {
        ESP_LOGI(TAG, "NDEF URI (len=%u)", (unsigned)rec.payload_len);
      }
    } else if (ndef_record_is_mime(&rec)) {
      log_str("MIME:", rec.type, rec.type_len);
      ESP_LOGI(TAG, "Payload len=%u", (unsigned)rec.payload_len);
    } else if (ndef_record_is_smart_poster(&rec)) {
      ESP_LOGI(TAG, "NDEF Smart Poster (payload=%u bytes)", (unsigned)rec.payload_len);
    } else {
      ESP_LOGI(TAG, "NDEF record type len=%u payload=%u", rec.type_len, (unsigned)rec.payload_len);
    }

    idx++;
    if (rec.me)
      break;
  }

  if (ndef_parser_error(&parser) != HB_NFC_OK) {
    ESP_LOGW(TAG, "NDEF parse error");
  }
}
