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

// Kantech ioProx XSF — 26-bit proprietary format
// Uses EM4100 customer code 0x08.
// Bits [31:24] = 8-bit family code
// Bits [23:16] = 8-bit facility code
// Bits [15:0]  = 16-bit card number

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_KANTECH";

#define KANTECH_BITS        40
#define KANTECH_CUSTOMER_ID 0x08

static bool protocol_kantech_decode(const ys_rfid2_raw_data_t *raw, rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  if (raw->data[0] != KANTECH_CUSTOMER_ID) {
    return false;
  }

  uint8_t facility_code = raw->data[2];
  uint16_t card_number = ((uint16_t)raw->data[3] << 8) | raw->data[4];

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  out_data->protocol_name = "Kantech ioProx";
  out_data->facility_code = facility_code;
  out_data->card_number = card_number;
  out_data->bit_count = KANTECH_BITS;
  out_data->raw_value = full_value;

  ESP_LOGD(TAG, "FC: %u, Card: %u", facility_code, card_number);
  return true;
}

rfid_protocol_t protocol_kantech = {
    .name = "Kantech ioProx",
    .decode = protocol_kantech_decode,
};
