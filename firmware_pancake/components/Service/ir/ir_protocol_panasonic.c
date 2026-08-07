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

#include "ir_protocol_panasonic.h"

#include "esp_log.h"

#include "ir_protocol.h"

static const char *TAG = "IR_PANASONIC";

bool ir_protocol_panasonic_decode(const rmt_symbol_word_t *symbols,
                                  size_t count,
                                  ir_data_t *out_data) {
  if (symbols == NULL || count == 0 || out_data == NULL)
    return false;

  if (count < PANASONIC_MIN_SYMBOLS)
    return false;
  if (!ir_match(symbols[0].duration0, PANASONIC_HEADER_MARK) ||
      !ir_match(symbols[0].duration1, PANASONIC_HEADER_SPACE))
    return false;

  ir_pulse_distance_cfg_t cfg = {
      .one_space = PANASONIC_ONE_SPACE,
      .zero_space = PANASONIC_ZERO_SPACE,
      .msb_first = false,
  };
  uint64_t raw = ir_decode_pulse_distance(symbols, 1, PANASONIC_FRAME_BITS, &cfg);

  uint16_t vendor = raw & PANASONIC_VENDOR_MASK;
  if (vendor != PANASONIC_VENDOR_ID)
    return false;

  uint8_t byte0 = (raw >> PANASONIC_BYTE0_SHIFT) & 0xFF;
  uint8_t byte1 = (raw >> PANASONIC_BYTE1_SHIFT) & 0xFF;
  uint8_t byte2 = (raw >> PANASONIC_BYTE2_SHIFT) & 0xFF;
  uint8_t byte3 = (raw >> PANASONIC_BYTE3_SHIFT) & 0xFF;

  if (byte3 != (uint8_t)(byte0 ^ byte1 ^ byte2))
    return false;

  out_data->protocol = IR_PROTO_PANASONIC;
  out_data->address = ((byte0 >> PANASONIC_NIBBLE_SHIFT) & PANASONIC_NIBBLE_MASK) |
                      ((uint16_t)byte1 << PANASONIC_NIBBLE_SHIFT);
  out_data->command = byte2;
  out_data->repeat = false;
  return true;
}

size_t ir_protocol_panasonic_encode(const ir_data_t *data, rmt_symbol_word_t *symbols, size_t max) {
  if (data == NULL || symbols == NULL || max == 0)
    return 0;

  uint16_t vendor = PANASONIC_VENDOR_ID;
  uint8_t vp = vendor ^ (vendor >> PANASONIC_BYTE_SHIFT);
  vp = (vp ^ (vp >> PANASONIC_NIBBLE_SHIFT)) & PANASONIC_NIBBLE_MASK;

  uint8_t byte0 = (vp & PANASONIC_NIBBLE_MASK) |
                  ((data->address & PANASONIC_NIBBLE_MASK) << PANASONIC_NIBBLE_SHIFT);
  uint8_t byte1 = (data->address >> PANASONIC_NIBBLE_SHIFT) & 0xFF;
  uint8_t byte2 = data->command & 0xFF;
  uint8_t byte3 = byte0 ^ byte1 ^ byte2;

  uint64_t raw = (uint64_t)vendor | ((uint64_t)byte0 << PANASONIC_BYTE0_SHIFT) |
                 ((uint64_t)byte1 << PANASONIC_BYTE1_SHIFT) |
                 ((uint64_t)byte2 << PANASONIC_BYTE2_SHIFT) |
                 ((uint64_t)byte3 << PANASONIC_BYTE3_SHIFT);

  ir_encode_distance_cfg_t cfg = {
      .header_mark = PANASONIC_HEADER_MARK,
      .header_space = PANASONIC_HEADER_SPACE,
      .bit_mark = PANASONIC_BIT_MARK,
      .one_space = PANASONIC_ONE_SPACE,
      .max = max,
      .zero_space = PANASONIC_ZERO_SPACE,
      .msb_first = false,
      .stop_bit = true,
  };
  return ir_encode_pulse_distance(symbols, raw, PANASONIC_FRAME_BITS, &cfg);
}