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

// PAC / Stanley — 26-bit Wiegand format
// Bit 25:     even parity (covers bits 25-13)
// Bits 24-17: 8-bit facility code (0-255)
// Bits 16-1:  16-bit card number (0-65535)
// Bit 0:      odd parity (covers bits 12-0)
// Distinguished from HID by EM4100 customer code 0x03.

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_PAC";

#define PAC_BITS        26
#define PAC_CUSTOMER_ID 0x03
#define PAC_FC_SHIFT    17
#define PAC_FC_MASK     0xFF
#define PAC_CARD_SHIFT  1
#define PAC_CARD_MASK   0xFFFF

static int count_bits(uint32_t value, int from_bit, int to_bit) {
  int count = 0;
  for (int i = from_bit; i <= to_bit; i++) {
    if (value & (1u << i)) {
      count++;
    }
  }
  return count;
}

static bool protocol_pac_stanley_decode(const ys_rfid2_raw_data_t *raw,
                                        rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  if (raw->data[0] != PAC_CUSTOMER_ID) {
    return false;
  }

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  uint32_t wiegand = (uint32_t)(full_value & 0x03FFFFFF);

  if ((count_bits(wiegand, 13, 25) % 2) != 0) {
    return false;
  }
  if ((count_bits(wiegand, 0, 12) % 2) != 1) {
    return false;
  }

  out_data->protocol_name = "PAC/Stanley";
  out_data->facility_code = (wiegand >> PAC_FC_SHIFT) & PAC_FC_MASK;
  out_data->card_number = (wiegand >> PAC_CARD_SHIFT) & PAC_CARD_MASK;
  out_data->bit_count = PAC_BITS;
  out_data->raw_value = wiegand;

  ESP_LOGD(TAG, "FC: %u, Card: %lu", out_data->facility_code, (unsigned long)out_data->card_number);
  return true;
}

rfid_protocol_t protocol_pac_stanley = {
    .name = "PAC/Stanley",
    .decode = protocol_pac_stanley_decode,
};
