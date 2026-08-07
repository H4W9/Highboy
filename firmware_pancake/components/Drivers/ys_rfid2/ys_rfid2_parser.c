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

#include "ys_rfid2_parser.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "RFID_PARSER";

#define PARSER_LINE_LEN   64
#define PARSER_PREFIX     "card number: "
#define PARSER_PREFIX_LEN 13
#define PARSER_DELIMITER  '@'

static char s_line_buf[PARSER_LINE_LEN];
static int s_line_pos = 0;

static bool validate_and_extract(char *out_id);
static void decimal_to_bytes(const char *id_str, uint8_t *out_data);

void ys_rfid2_parser_reset(void) {
  s_line_pos = 0;
}

bool ys_rfid2_parser_feed(uint8_t byte, ys_rfid2_raw_data_t *out_raw) {
  if (out_raw == NULL) {
    return false;
  }

  if (byte == PARSER_DELIMITER) {
    char card_id[YS_RFID2_CARD_ID_LEN + 1];
    bool is_valid = validate_and_extract(card_id);
    s_line_pos = 0;

    if (is_valid) {
      memcpy(out_raw->id_str, card_id, YS_RFID2_CARD_ID_LEN + 1);
      decimal_to_bytes(card_id, out_raw->data);
      out_raw->bit_count = 40;
      ESP_LOGD(TAG, "Card parsed: %s", card_id);
      return true;
    }

    return false;
  }

  if (s_line_pos < PARSER_LINE_LEN - 1) {
    s_line_buf[s_line_pos++] = (char)byte;
  } else {
    s_line_pos = 0;
  }

  return false;
}

static bool validate_and_extract(char *out_id) {
  if (s_line_pos < PARSER_PREFIX_LEN + YS_RFID2_CARD_ID_LEN) {
    return false;
  }

  const char *prefix_pos = NULL;
  int search_limit = s_line_pos - (PARSER_PREFIX_LEN + YS_RFID2_CARD_ID_LEN);
  for (int i = 0; i <= search_limit; i++) {
    if (memcmp(&s_line_buf[i], PARSER_PREFIX, PARSER_PREFIX_LEN) == 0) {
      prefix_pos = &s_line_buf[i];
      break;
    }
  }

  if (prefix_pos == NULL) {
    return false;
  }

  const char *digits = prefix_pos + PARSER_PREFIX_LEN;
  for (int i = 0; i < YS_RFID2_CARD_ID_LEN; i++) {
    if (digits[i] < '0' || digits[i] > '9') {
      return false;
    }
  }

  memcpy(out_id, digits, YS_RFID2_CARD_ID_LEN);
  out_id[YS_RFID2_CARD_ID_LEN] = '\0';
  return true;
}

// Convert 10-digit decimal string to 5 raw bytes (40 bits big-endian).
// Max value of 10 decimal digits = 9999999999 which fits in 40 bits.
static void decimal_to_bytes(const char *id_str, uint8_t *out_data) {
  uint64_t value = 0;
  for (int i = 0; i < YS_RFID2_CARD_ID_LEN; i++) {
    value = value * 10 + (uint64_t)(id_str[i] - '0');
  }

  out_data[0] = (uint8_t)(value >> 32);
  out_data[1] = (uint8_t)(value >> 24);
  out_data[2] = (uint8_t)(value >> 16);
  out_data[3] = (uint8_t)(value >> 8);
  out_data[4] = (uint8_t)(value);
}
