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

#include "ir_ac_coolix.h"

#include "ir_protocol.h"

#define COOLIX_DEFAULT_STATE 0xB21FC8u
#define COOLIX_OFF_STATE     0xB27BE0u

#define COOLIX_MODE_SHIFT 2
#define COOLIX_MODE_MASK  0x3u
#define COOLIX_TEMP_SHIFT 4
#define COOLIX_TEMP_MASK  0xFu
#define COOLIX_FAN_SHIFT  13
#define COOLIX_FAN_MASK   0x7u

#define COOLIX_MODE_COOL 0x0
#define COOLIX_MODE_DRY  0x1
#define COOLIX_MODE_AUTO 0x2
#define COOLIX_MODE_HEAT 0x3

#define COOLIX_FAN_AUTO  0x5
#define COOLIX_FAN_AUTO0 0x0
#define COOLIX_FAN_MIN   0x4
#define COOLIX_FAN_MED   0x2
#define COOLIX_FAN_MAX   0x1

#define COOLIX_TEMP_MIN      17
#define COOLIX_TEMP_MAX      30
#define COOLIX_FAN_TEMP_CODE 0xEu

#define COOLIX_WIRE_BITS 48

static const uint8_t COOLIX_TEMP_MAP[] = {
    0x0, 0x1, 0x3, 0x2, 0x6, 0x7, 0x5, 0x4, 0xC, 0xD, 0x9, 0x8, 0xA, 0xB};
#define COOLIX_TEMP_MAP_COUNT (sizeof(COOLIX_TEMP_MAP) / sizeof(COOLIX_TEMP_MAP[0]))

size_t ir_ac_coolix_encode(const ir_ac_state_t *state, rmt_symbol_word_t *symbols, size_t max) {
  if (state == NULL || symbols == NULL || max == 0)
    return 0;

  uint32_t raw;
  if (!state->power) {
    raw = COOLIX_OFF_STATE;
  } else {
    raw = COOLIX_DEFAULT_STATE;

    bool is_fan_only = false;
    uint8_t mode_code;
    switch (state->mode) {
      case IR_AC_MODE_COOL:
        mode_code = COOLIX_MODE_COOL;
        break;
      case IR_AC_MODE_DRY:
        mode_code = COOLIX_MODE_DRY;
        break;
      case IR_AC_MODE_HEAT:
        mode_code = COOLIX_MODE_HEAT;
        break;
      case IR_AC_MODE_FAN:
        mode_code = COOLIX_MODE_DRY;
        is_fan_only = true;
        break;
      case IR_AC_MODE_AUTO:
      default:
        mode_code = COOLIX_MODE_AUTO;
        break;
    }
    raw = (raw & ~(COOLIX_MODE_MASK << COOLIX_MODE_SHIFT)) |
          ((uint32_t)mode_code << COOLIX_MODE_SHIFT);

    uint8_t temp = state->temp_c;
    if (temp < COOLIX_TEMP_MIN)
      temp = COOLIX_TEMP_MIN;
    if (temp > COOLIX_TEMP_MAX)
      temp = COOLIX_TEMP_MAX;
    uint8_t temp_code =
        is_fan_only ? COOLIX_FAN_TEMP_CODE : COOLIX_TEMP_MAP[temp - COOLIX_TEMP_MIN];
    raw = (raw & ~(COOLIX_TEMP_MASK << COOLIX_TEMP_SHIFT)) |
          ((uint32_t)temp_code << COOLIX_TEMP_SHIFT);

    uint8_t fan_code;
    switch (state->fan) {
      case IR_AC_FAN_LOW:
        fan_code = COOLIX_FAN_MIN;
        break;
      case IR_AC_FAN_MED:
        fan_code = COOLIX_FAN_MED;
        break;
      case IR_AC_FAN_HIGH:
        fan_code = COOLIX_FAN_MAX;
        break;
      case IR_AC_FAN_AUTO:
      default:
        fan_code = (state->mode == IR_AC_MODE_AUTO || state->mode == IR_AC_MODE_DRY)
                       ? COOLIX_FAN_AUTO0
                       : COOLIX_FAN_AUTO;
        break;
    }
    raw = (raw & ~(COOLIX_FAN_MASK << COOLIX_FAN_SHIFT)) | ((uint32_t)fan_code << COOLIX_FAN_SHIFT);
  }

  uint64_t wire = 0;
  for (int i = 2; i >= 0; i--) {
    uint8_t byte = (raw >> (i * 8)) & 0xFF;
    wire = (wire << 8) | byte;
    wire = (wire << 8) | (uint8_t)(~byte);
  }

  ir_encode_distance_cfg_t cfg = {
      .header_mark = COOLIX_HDR_MARK,
      .header_space = COOLIX_HDR_SPACE,
      .bit_mark = COOLIX_BIT_MARK,
      .one_space = COOLIX_ONE_SPACE,
      .zero_space = COOLIX_ZERO_SPACE,
      .max = max,
      .msb_first = true,
      .stop_bit = true,
  };
  return ir_encode_pulse_distance(symbols, wire, COOLIX_WIRE_BITS, &cfg);
}
