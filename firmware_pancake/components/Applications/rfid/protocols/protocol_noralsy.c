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

// Noralsy — French access control system
// Uses EM4100 customer code 0x0B.
// Lower 32 bits:
// Bits [31:16] = 16-bit site code
// Bits [15:0]  = 16-bit card number

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_NORALSY";

#define NORALSY_BITS        40
#define NORALSY_CUSTOMER_ID 0x0B

static bool protocol_noralsy_decode(const ys_rfid2_raw_data_t *raw, rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  if (raw->data[0] != NORALSY_CUSTOMER_ID) {
    return false;
  }

  uint16_t site_code = ((uint16_t)raw->data[1] << 8) | raw->data[2];
  uint16_t card_number = ((uint16_t)raw->data[3] << 8) | raw->data[4];

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  out_data->protocol_name = "Noralsy";
  out_data->facility_code = site_code;
  out_data->card_number = card_number;
  out_data->bit_count = NORALSY_BITS;
  out_data->raw_value = full_value;

  ESP_LOGD(TAG, "Site: %u, Card: %u", site_code, card_number);
  return true;
}

rfid_protocol_t protocol_noralsy = {
    .name = "Noralsy",
    .decode = protocol_noralsy_decode,
};
