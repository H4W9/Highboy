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
 * @file ndef.h
 * @brief NDEF parser/builder helpers.
 */
#ifndef NDEF_H
#define NDEF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TNF values */
#define NDEF_TNF_EMPTY      0x00U
#define NDEF_TNF_WELL_KNOWN 0x01U
#define NDEF_TNF_MIME       0x02U
#define NDEF_TNF_URI        0x03U
#define NDEF_TNF_EXTERNAL   0x04U
#define NDEF_TNF_UNKNOWN    0x05U
#define NDEF_TNF_UNCHANGED  0x06U
#define NDEF_TNF_RESERVED   0x07U

/* TLV types (Type 1/2/3 Tag mappings) */
#define NDEF_TLV_NULL        0x00U
#define NDEF_TLV_LOCK_CTRL   0x01U
#define NDEF_TLV_MEM_CTRL    0x02U
#define NDEF_TLV_NDEF        0x03U
#define NDEF_TLV_PROPRIETARY 0xFD
#define NDEF_TLV_TERMINATOR  0xFEU

/**
 * @brief Parsed NDEF record.
 */
typedef struct {
  bool mb;                /**< @brief Message Begin flag. */
  bool me;                /**< @brief Message End flag. */
  bool cf;                /**< @brief Chunk Flag. */
  bool sr;                /**< @brief Short Record flag. */
  bool il;                /**< @brief ID Length present flag. */
  uint8_t tnf;            /**< @brief Type Name Format. */
  uint8_t type_len;       /**< @brief Type field length. */
  uint8_t id_len;         /**< @brief ID field length. */
  uint32_t payload_len;   /**< @brief Payload length. */
  const uint8_t *type;    /**< @brief Pointer to type field. */
  const uint8_t *id;      /**< @brief Pointer to ID field. */
  const uint8_t *payload; /**< @brief Pointer to payload. */
} ndef_record_t;

/**
 * @brief NDEF message parser state.
 */
typedef struct {
  const uint8_t *data; /**< @brief Raw NDEF data. */
  size_t len;          /**< @brief Total data length. */
  size_t pos;          /**< @brief Current parse position. */
  bool done;           /**< @brief True when parsing is complete. */
  hb_nfc_err_t err;    /**< @brief Error code from last operation. */
} ndef_parser_t;

/**
 * @brief Decoded NDEF Text record payload.
 */
typedef struct {
  const uint8_t *lang; /**< @brief Language code bytes. */
  uint8_t lang_len;    /**< @brief Language code length. */
  const uint8_t *text; /**< @brief Text content bytes. */
  size_t text_len;     /**< @brief Text content length. */
  bool utf16;          /**< @brief True if UTF-16 encoded. */
} ndef_text_t;

/**
 * @brief Record data for ndef_builder_add_record().
 */
typedef struct {
  uint8_t tnf;            /**< @brief Type Name Format. */
  const uint8_t *type;    /**< @brief Type field data. */
  uint8_t type_len;       /**< @brief Type field length. */
  const uint8_t *id;      /**< @brief ID field data (NULL if none). */
  uint8_t id_len;         /**< @brief ID field length. */
  const uint8_t *payload; /**< @brief Payload data. */
  uint32_t payload_len;   /**< @brief Payload length. */
} ndef_record_data_t;

/**
 * @brief NDEF message builder state.
 */
typedef struct {
  uint8_t *buf;        /**< @brief Output buffer. */
  size_t max;          /**< @brief Maximum buffer size. */
  size_t len;          /**< @brief Current written length. */
  size_t last_hdr_off; /**< @brief Offset of last record header. */
  bool has_record;     /**< @brief True if at least one record added. */
} ndef_builder_t;

/**
 * @brief Parsed TLV entry.
 */
typedef struct {
  uint8_t type;         /**< @brief TLV type byte. */
  size_t length;        /**< @brief TLV value length. */
  const uint8_t *value; /**< @brief Pointer to TLV value. */
  size_t tlv_len;       /**< @brief Total TLV structure length. */
} ndef_tlv_t;

/**
 * @brief TLV search/build status codes.
 */
typedef enum {
  NDEF_TLV_STATUS_OK = 0,        /**< @brief Success. */
  NDEF_TLV_STATUS_NOT_FOUND = 1, /**< @brief TLV not found. */
  NDEF_TLV_STATUS_INVALID = 2    /**< @brief Invalid TLV data. */
} ndef_tlv_status_t;

/* Parser helpers */

/**
 * @brief Initialize an NDEF parser.
 *
 * @param[out] parser  Parser state to initialize.
 * @param data         Raw NDEF data.
 * @param len          Length of data in bytes.
 */
void ndef_parser_init(ndef_parser_t *parser, const uint8_t *data, size_t len);

/**
 * @brief Parse the next NDEF record.
 *
 * @param parser       Parser state.
 * @param[out] rec     Parsed record output.
 * @return
 *   - true if a record was parsed
 *   - false if no more records or on error
 */
bool ndef_parse_next(ndef_parser_t *parser, ndef_record_t *rec);

/**
 * @brief Get the last parser error code.
 *
 * @param parser  Parser state.
 * @return Last error code from parsing.
 */
hb_nfc_err_t ndef_parser_error(const ndef_parser_t *parser);

/**
 * @brief Check if a record is an NDEF Text record.
 *
 * @param rec  Record to check.
 * @return
 *   - true if record is a Text record
 *   - false otherwise
 */
bool ndef_record_is_text(const ndef_record_t *rec);

/**
 * @brief Check if a record is an NDEF URI record.
 *
 * @param rec  Record to check.
 * @return
 *   - true if record is a URI record
 *   - false otherwise
 */
bool ndef_record_is_uri(const ndef_record_t *rec);

/**
 * @brief Check if a record is a Smart Poster record.
 *
 * @param rec  Record to check.
 * @return
 *   - true if record is a Smart Poster
 *   - false otherwise
 */
bool ndef_record_is_smart_poster(const ndef_record_t *rec);

/**
 * @brief Check if a record is a MIME type record.
 *
 * @param rec  Record to check.
 * @return
 *   - true if record is a MIME record
 *   - false otherwise
 */
bool ndef_record_is_mime(const ndef_record_t *rec);

/**
 * @brief Decode an NDEF Text record payload.
 *
 * @param rec      Text record to decode.
 * @param[out] out Decoded text data.
 * @return
 *   - true on success
 *   - false if record is not a valid Text record
 */
bool ndef_decode_text(const ndef_record_t *rec, ndef_text_t *out);

/**
 * @brief Decode an NDEF URI record payload.
 *
 * @param rec          URI record to decode.
 * @param[out] out     Buffer for decoded URI string.
 * @param out_max      Maximum output buffer size.
 * @param[out] out_len Actual URI length written.
 * @return
 *   - true on success
 *   - false if record is not a valid URI record
 */
bool ndef_decode_uri(const ndef_record_t *rec, char *out, size_t out_max, size_t *out_len);

/* Builder helpers */

/**
 * @brief Initialize an NDEF builder.
 *
 * @param[out] b  Builder state to initialize.
 * @param out     Output buffer for NDEF data.
 * @param max     Maximum buffer size.
 */
void ndef_builder_init(ndef_builder_t *b, uint8_t *out, size_t max);

/**
 * @brief Get current length of built NDEF message.
 *
 * @param b  Builder state.
 * @return Number of bytes written so far.
 */
size_t ndef_builder_len(const ndef_builder_t *b);

/**
 * @brief Add a raw NDEF record.
 *
 * @param b    Builder state.
 * @param rec  Record data to add.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_NO_MEM if buffer is full
 */
hb_nfc_err_t ndef_builder_add_record(ndef_builder_t *b, const ndef_record_data_t *rec);

/**
 * @brief Add an NDEF Text record.
 *
 * @param b      Builder state.
 * @param text   Text content string.
 * @param lang   Language code string (e.g. "en").
 * @param utf16  True to use UTF-16 encoding.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_NO_MEM if buffer is full
 */
hb_nfc_err_t
ndef_builder_add_text(ndef_builder_t *b, const char *text, const char *lang, bool utf16);

/**
 * @brief Add an NDEF URI record.
 *
 * @param b    Builder state.
 * @param uri  URI string.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_NO_MEM if buffer is full
 */
hb_nfc_err_t ndef_builder_add_uri(ndef_builder_t *b, const char *uri);

/**
 * @brief Add an NDEF MIME type record.
 *
 * @param b          Builder state.
 * @param mime_type  MIME type string (e.g. "text/plain").
 * @param data       Payload data.
 * @param data_len   Payload length.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_NO_MEM if buffer is full
 */
hb_nfc_err_t ndef_builder_add_mime(ndef_builder_t *b,
                                   const char *mime_type,
                                   const uint8_t *data,
                                   size_t data_len);

/**
 * @brief Add an NDEF Smart Poster record.
 *
 * @param b           Builder state.
 * @param nested_ndef Nested NDEF message data.
 * @param nested_len  Length of nested NDEF data.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_NO_MEM if buffer is full
 */
hb_nfc_err_t
ndef_builder_add_smart_poster(ndef_builder_t *b, const uint8_t *nested_ndef, size_t nested_len);

/* TLV helpers (Type 1/2/3 mappings) */

/**
 * @brief Find a TLV entry by type in raw data.
 *
 * @param data     Raw tag data.
 * @param len      Data length.
 * @param type     TLV type to search for.
 * @param[out] out Found TLV entry.
 * @return TLV search status code.
 */
ndef_tlv_status_t ndef_tlv_find(const uint8_t *data, size_t len, uint8_t type, ndef_tlv_t *out);

/**
 * @brief Find the NDEF TLV entry in raw data.
 *
 * @param data     Raw tag data.
 * @param len      Data length.
 * @param[out] out Found NDEF TLV entry.
 * @return TLV search status code.
 */
ndef_tlv_status_t ndef_tlv_find_ndef(const uint8_t *data, size_t len, ndef_tlv_t *out);

/**
 * @brief Build a TLV structure into a buffer.
 *
 * @param[out] out      Output buffer.
 * @param      max      Maximum buffer size.
 * @param      tlv      TLV data (type, value, length fields).
 * @param[out] out_len  Total bytes written.
 * @return TLV build status code.
 */
ndef_tlv_status_t ndef_tlv_build(uint8_t *out, size_t max, const ndef_tlv_t *tlv, size_t *out_len);

/**
 * @brief Build an NDEF TLV with terminator into a buffer.
 *
 * @param[out] out      Output buffer.
 * @param      max      Maximum buffer size.
 * @param      tlv      TLV data (type, value, length fields).
 * @param[out] out_len  Total bytes written.
 * @return TLV build status code.
 */
ndef_tlv_status_t
ndef_tlv_build_ndef(uint8_t *out, size_t max, const ndef_tlv_t *tlv, size_t *out_len);

/**
 * @brief Print NDEF records to log (best-effort).
 *
 * @param data  Raw NDEF data.
 * @param len   Data length.
 */
void ndef_print(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NDEF_H */
