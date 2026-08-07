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

// Gallagher — Enterprise access control
// Uses EM4100 customer code 0x09.
// Data layout in lower 32 bits:
// Bits [31:24] = 8-bit region code
// Bits [23:8]  = 16-bit cardholder number
// Bits [7:4]   = 4-bit issue level
// Bits [3:0]   = 4-bit checksum nibble
// Ref: https://github.com/megabug/gallagher-research

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_GALLAGHER";

#define GALLAGHER_BITS        40
#define GALLAGHER_CUSTOMER_ID 0x09
#define GALLAGHER_REGION_MASK 0xFF
#define GALLAGHER_CARD_MASK   0xFFFF
#define GALLAGHER_ISSUE_MASK  0x0F

static bool protocol_gallagher_decode(const ys_rfid2_raw_data_t *raw,
                                      rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  if (raw->data[0] != GALLAGHER_CUSTOMER_ID) {
    return false;
  }

  uint8_t region = raw->data[1];
  uint16_t card_number = ((uint16_t)raw->data[2] << 8) | raw->data[3];
  uint8_t issue_level = (raw->data[4] >> 4) & GALLAGHER_ISSUE_MASK;
  uint8_t checksum = raw->data[4] & 0x0F;

  // Simple checksum: XOR of all nibbles should produce a known value
  uint8_t calc = 0;
  for (int i = 0; i < 4; i++) {
    calc ^= (raw->data[i] >> 4) ^ (raw->data[i] & 0x0F);
  }
  calc ^= (raw->data[4] >> 4);

  if ((calc & 0x0F) != checksum) {
    return false;
  }

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  out_data->protocol_name = "Gallagher";
  out_data->facility_code = region;
  out_data->card_number = card_number;
  out_data->bit_count = GALLAGHER_BITS;
  out_data->raw_value = full_value;

  ESP_LOGD(TAG, "Region: %u, Card: %u, Issue: %u", region, card_number, issue_level);
  return true;
}

rfid_protocol_t protocol_gallagher = {
    .name = "Gallagher",
    .decode = protocol_gallagher_decode,
};
