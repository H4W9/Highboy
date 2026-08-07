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

// FDX-B — ISO 11784/11785 Animal Identification
// 134.2kHz carrier (may not be directly readable by 125kHz YS-RFID2)
// When raw 40-bit data is available, attempt to extract:
// - 10-bit country code
// - 38-bit national ID (lower 32 bits stored as card_number)
// This is a best-effort decoder for cloned/compatible data.
// Ref: https://www.priority1design.com.au/fdx-b_animal_identification_protocol.html

#include "rfid_protocol_decoder.h"

#include "esp_log.h"

static const char *TAG = "PROTO_FDXB";

#define FDXB_BITS          40
#define FDXB_COUNTRY_MASK  0x3FF
#define FDXB_COUNTRY_SHIFT 32

static bool protocol_fdxb_decode(const ys_rfid2_raw_data_t *raw, rfid_decoded_data_t *out_data) {
  if (raw->bit_count != 40) {
    return false;
  }

  uint64_t full_value = 0;
  for (int i = 0; i < YS_RFID2_RAW_DATA_LEN; i++) {
    full_value = (full_value << 8) | raw->data[i];
  }

  // Upper 8 bits as country hint (only 10 bits in full FDX-B, we have 8)
  uint16_t country_code = (uint16_t)((full_value >> FDXB_COUNTRY_SHIFT) & FDXB_COUNTRY_MASK);
  uint32_t national_id = (uint32_t)(full_value & 0xFFFFFFFF);

  // FDX-B country codes are in range 1-999 (ISO 3166)
  // If the upper byte doesn't look like a valid country code range, skip
  if (country_code == 0 || country_code > 999) {
    return false;
  }

  out_data->protocol_name = "FDX-B";
  out_data->facility_code = country_code;
  out_data->card_number = national_id;
  out_data->bit_count = FDXB_BITS;
  out_data->raw_value = full_value;

  ESP_LOGD(TAG, "Country: %u, ID: %lu", country_code, (unsigned long)national_id);
  return true;
}

rfid_protocol_t protocol_fdxb = {
    .name = "FDX-B",
    .decode = protocol_fdxb_decode,
};
