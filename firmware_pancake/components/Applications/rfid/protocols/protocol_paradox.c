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

// Paradox — Proprietary security system format
// Uses the full 40 bits from EM4100:
// Bits [39:32] = 8-bit facility/site code
// Bits [31:16] = 16-bit card number
// Bits [15:0]  = 16-bit checksum/serial
// Distinguished by EM4100 customer code 0x05.

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_PARADOX";

#define PARADOX_BITS        40
#define PARADOX_CUSTOMER_ID 0x05
#define PARADOX_FC_SHIFT    24
#define PARADOX_FC_MASK     0xFF
#define PARADOX_CARD_SHIFT  8
#define PARADOX_CARD_MASK   0xFFFF

static bool protocol_paradox_decode(const ys_rfid2_raw_data_t *raw, rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  if (raw->data[0] != PARADOX_CUSTOMER_ID) {
    return false;
  }

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  // Strip customer code byte, work with lower 32 bits
  uint32_t payload = (uint32_t)(full_value & 0xFFFFFFFF);

  out_data->protocol_name = "Paradox";
  out_data->facility_code = (payload >> PARADOX_FC_SHIFT) & PARADOX_FC_MASK;
  out_data->card_number = (payload >> PARADOX_CARD_SHIFT) & PARADOX_CARD_MASK;
  out_data->bit_count = PARADOX_BITS;
  out_data->raw_value = full_value;

  ESP_LOGD(TAG, "FC: %u, Card: %lu", out_data->facility_code, (unsigned long)out_data->card_number);
  return true;
}

rfid_protocol_t protocol_paradox = {
    .name = "Paradox",
    .decode = protocol_paradox_decode,
};
