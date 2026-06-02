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

#include "ir_ac_gree.h"

#include "ir_protocol.h"

#define GREE_MODE_AUTO 0
#define GREE_MODE_COOL 1
#define GREE_MODE_DRY  2
#define GREE_MODE_FAN  3
#define GREE_MODE_HEAT 4

#define GREE_FAN_AUTO 0
#define GREE_FAN_MIN  1
#define GREE_FAN_MED  2
#define GREE_FAN_MAX  3

#define GREE_TEMP_MIN  16
#define GREE_TEMP_MAX  30
#define GREE_AUTO_TEMP 25

#define GREE_CHECKSUM_START 10
#define GREE_BLOCK_BITS     32

static size_t append_footer(rmt_symbol_word_t *symbols, size_t idx, size_t max) {
  if (idx + 1 > max)
    return 0;
  symbols[idx].duration0 = GREE_BIT_MARK;
  symbols[idx].level0 = 1;
  symbols[idx].duration1 = GREE_MSG_SPACE;
  symbols[idx].level1 = 0;
  return idx + 1;
}

size_t ir_ac_gree_encode(const ir_ac_state_t *state, rmt_symbol_word_t *symbols, size_t max) {
  if (state == NULL || symbols == NULL || max == 0)
    return 0;

  uint8_t state_bytes[GREE_STATE_LEN] = {0x00, 0x09, 0x20, 0x50, 0x00, 0x20, 0x00, 0x00};

  uint8_t mode_code;
  switch (state->mode) {
    case IR_AC_MODE_COOL:
      mode_code = GREE_MODE_COOL;
      break;
    case IR_AC_MODE_DRY:
      mode_code = GREE_MODE_DRY;
      break;
    case IR_AC_MODE_HEAT:
      mode_code = GREE_MODE_HEAT;
      break;
    case IR_AC_MODE_FAN:
      mode_code = GREE_MODE_FAN;
      break;
    case IR_AC_MODE_AUTO:
    default:
      mode_code = GREE_MODE_AUTO;
      break;
  }

  uint8_t fan_code;
  switch (state->fan) {
    case IR_AC_FAN_LOW:
      fan_code = GREE_FAN_MIN;
      break;
    case IR_AC_FAN_MED:
      fan_code = GREE_FAN_MED;
      break;
    case IR_AC_FAN_HIGH:
      fan_code = GREE_FAN_MAX;
      break;
    case IR_AC_FAN_AUTO:
    default:
      fan_code = GREE_FAN_AUTO;
      break;
  }
  if (state->mode == IR_AC_MODE_DRY)
    fan_code = GREE_FAN_MIN;

  uint8_t temp = (state->mode == IR_AC_MODE_AUTO) ? GREE_AUTO_TEMP : state->temp_c;
  if (temp < GREE_TEMP_MIN)
    temp = GREE_TEMP_MIN;
  if (temp > GREE_TEMP_MAX)
    temp = GREE_TEMP_MAX;

  state_bytes[0] = (mode_code & 0x7) | ((state->power ? 1u : 0u) << 3) | ((fan_code & 0x3) << 4);
  state_bytes[1] = (state_bytes[1] & 0xF0) | ((uint8_t)(temp - GREE_TEMP_MIN) & 0x0F);

  uint8_t sum = GREE_CHECKSUM_START;
  for (size_t i = 0; i < 4; i++)
    sum += state_bytes[i] & 0x0F;
  for (size_t i = 4; i < GREE_STATE_LEN - 1; i++)
    sum += state_bytes[i] >> 4;
  sum &= 0x0F;
  state_bytes[GREE_STATE_LEN - 1] = (state_bytes[GREE_STATE_LEN - 1] & 0x0F) | (uint8_t)(sum << 4);

  uint32_t block1 = (uint32_t)state_bytes[0] | ((uint32_t)state_bytes[1] << 8) |
                    ((uint32_t)state_bytes[2] << 16) | ((uint32_t)state_bytes[3] << 24);
  uint32_t block2 = (uint32_t)state_bytes[4] | ((uint32_t)state_bytes[5] << 8) |
                    ((uint32_t)state_bytes[6] << 16) | ((uint32_t)state_bytes[7] << 24);

  ir_encode_distance_cfg_t cfg = {
      .header_mark = GREE_HDR_MARK,
      .header_space = GREE_HDR_SPACE,
      .bit_mark = GREE_BIT_MARK,
      .one_space = GREE_ONE_SPACE,
      .zero_space = GREE_ZERO_SPACE,
      .max = max,
      .msb_first = false,
      .stop_bit = false,
  };

  size_t idx = 0;
  size_t n = ir_encode_pulse_distance(symbols, block1, GREE_BLOCK_BITS, &cfg);
  if (n == 0)
    return 0;
  idx += n;

  cfg.header_mark = 0;
  cfg.header_space = 0;
  cfg.max = max - idx;
  n = ir_encode_pulse_distance(symbols + idx, GREE_BLOCK_FOOTER, GREE_BLOCK_FOOTER_BITS, &cfg);
  if (n == 0)
    return 0;
  idx += n;

  idx = append_footer(symbols, idx, max);
  if (idx == 0)
    return 0;

  cfg.max = max - idx;
  n = ir_encode_pulse_distance(symbols + idx, block2, GREE_BLOCK_BITS, &cfg);
  if (n == 0)
    return 0;
  idx += n;

  idx = append_footer(symbols, idx, max);
  if (idx == 0)
    return 0;

  return idx;
}
