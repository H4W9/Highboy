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

// HID Corporate 1000 H10302 — 35-bit format
// Bit 34:     even parity (covers bits 34-18)
// Bits 33-22: 12-bit company/site code (0-4095)
// Bits 21-2:  20-bit card number (0-1048575)
// Bit 1:      odd parity (covers bits 17-1)
// Bit 0:      odd parity (covers bits 34-2, full frame)

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_HID_CORP";

#define HID_CORP_BITS       35
#define HID_CORP_SITE_SHIFT 22
#define HID_CORP_SITE_MASK  0xFFF
#define HID_CORP_CARD_SHIFT 2
#define HID_CORP_CARD_MASK  0xFFFFF

static int count_bits_64(uint64_t value, int from_bit, int to_bit) {
  int count = 0;
  for (int i = from_bit; i <= to_bit; i++) {
    if (value & (1ULL << i)) {
      count++;
    }
  }
  return count;
}

static bool protocol_hid_corp_decode(const ys_rfid2_raw_data_t *raw,
                                     rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  // Use lower 35 bits
  uint64_t corp = full_value & 0x7FFFFFFFFULL;

  // Extract fields
  uint16_t site_code = (uint16_t)((corp >> HID_CORP_SITE_SHIFT) & HID_CORP_SITE_MASK);
  uint32_t card_number = (uint32_t)((corp >> HID_CORP_CARD_SHIFT) & HID_CORP_CARD_MASK);

  // Validate even parity (bit 34 covers bits 34-18)
  int even_count = count_bits_64(corp, 18, 34);
  if ((even_count % 2) != 0) {
    return false;
  }

  // Validate odd parity (bit 1 covers bits 17-1)
  int odd_count = count_bits_64(corp, 1, 17);
  if ((odd_count % 2) != 1) {
    return false;
  }

  // Validate frame parity (bit 0 covers bits 34-2)
  int frame_count = count_bits_64(corp, 0, 34);
  if ((frame_count % 2) != 1) {
    return false;
  }

  out_data->protocol_name = "HID Corp 1000";
  out_data->facility_code = site_code;
  out_data->card_number = card_number;
  out_data->bit_count = HID_CORP_BITS;
  out_data->raw_value = corp;

  ESP_LOGD(TAG, "Site: %u, Card: %lu", site_code, (unsigned long)card_number);
  return true;
}

rfid_protocol_t protocol_hid_corp = {
    .name = "HID Corp 1000",
    .decode = protocol_hid_corp_decode,
};
