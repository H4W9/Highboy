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

#include "ir_protocol_rc5.h"

#include "esp_log.h"

#include "ir_protocol.h"

typedef struct {
  uint32_t *buf;
  size_t len;
  size_t pos;
  uint32_t units;
  uint32_t unit;
} ir_protocol_rc5_biphase_t;

typedef struct {
  rmt_symbol_word_t *buf;
  size_t count;
  uint32_t mark_us;
  uint32_t space_us;
} ir_protocol_rc5_menc_t;

static const char *TAG = "IR_RC5";

static void bp_init(ir_protocol_rc5_biphase_t *bp, uint32_t *buf, size_t len, uint32_t unit) {
  bp->buf = buf;
  bp->len = len;
  bp->pos = 0;
  bp->unit = unit;
  bp->units = (len > 0) ? (buf[0] + unit / 2) / unit : 0;
}

static int bp_get(ir_protocol_rc5_biphase_t *bp) {
  while (bp->pos < bp->len && bp->units == 0) {
    bp->pos++;
    if (bp->pos < bp->len)
      bp->units = (bp->buf[bp->pos] + bp->unit / 2) / bp->unit;
  }
  if (bp->pos >= bp->len)
    return -1;
  bp->units--;
  return (bp->pos % 2 == 0) ? 1 : 0;
}

static void me_init(ir_protocol_rc5_menc_t *me, rmt_symbol_word_t *buf) {
  me->buf = buf;
  me->count = 0;
  me->mark_us = 0;
  me->space_us = 0;
}

static void me_mark(ir_protocol_rc5_menc_t *me, uint32_t us) {
  if (me->space_us > 0) {
    me->buf[me->count].duration0 = me->mark_us;
    me->buf[me->count].level0 = 1;
    me->buf[me->count].duration1 = me->space_us;
    me->buf[me->count].level1 = 0;
    me->count++;
    me->mark_us = us;
    me->space_us = 0;
  } else {
    me->mark_us += us;
  }
}

static void me_space(ir_protocol_rc5_menc_t *me, uint32_t us) {
  me->space_us += us;
}

static size_t me_finish(ir_protocol_rc5_menc_t *me) {
  if (me->mark_us > 0 || me->space_us > 0) {
    me->buf[me->count].duration0 = me->mark_us;
    me->buf[me->count].level0 = 1;
    me->buf[me->count].duration1 = me->space_us;
    me->buf[me->count].level1 = 0;
    me->count++;
  }
  return me->count;
}

bool ir_protocol_rc5_decode(const rmt_symbol_word_t *symbols, size_t count, ir_data_t *out_data) {
  if (symbols == NULL || count == 0 || out_data == NULL)
    return false;

  if (!ir_match(symbols[0].duration0, RC5_UNIT) && !ir_match(symbols[0].duration0, RC5_UNIT * 2))
    return false;

  uint32_t flat[RC5_FLAT_BUF_SIZE];
  size_t flat_len = 0;
  for (size_t i = 0; i < count && flat_len < RC5_FLAT_BUF_MAX; i++) {
    if (symbols[i].duration0)
      flat[flat_len++] = symbols[i].duration0;
    if (symbols[i].duration1)
      flat[flat_len++] = symbols[i].duration1;
  }

  ir_protocol_rc5_biphase_t bp;
  bp_init(&bp, flat, flat_len, RC5_UNIT);

  if (bp_get(&bp) != 1)
    return false;

  uint32_t raw = 0;
  int decoded = 0;

  for (int i = 0; i < RC5_DECODE_BITS; i++) {
    int a = bp_get(&bp);
    int b = bp_get(&bp);
    if (a < 0 || b < 0)
      break;

    if (a == 0 && b == 1)
      raw |= (1UL << (RC5_DECODE_BITS - 1 - i));
    else if (!(a == 1 && b == 0))
      break;

    decoded++;
  }

  if (decoded < RC5_MIN_DECODED_BITS)
    return false;

  out_data->protocol = IR_PROTO_RC5;
  out_data->command = raw & RC5_CMD_MASK;
  out_data->address = (raw >> RC5_ADDR_SHIFT) & RC5_ADDR_MASK;
  out_data->repeat = false;

  if (!((raw >> RC5_DECODE_BITS) & 1)) {
    out_data->command |= RC5X_CMD_BIT;
  }

  return true;
}

size_t ir_protocol_rc5_encode(const ir_data_t *data, rmt_symbol_word_t *symbols, size_t max) {
  if (data == NULL || symbols == NULL || max == 0)
    return 0;

  if (max < RC5_FLAT_BUF_SIZE) {
    ESP_LOGE(TAG, "Buffer too small for RC5 frame");
    return 0;
  }

  // NOTE: not thread-safe. Must be called from a single task.
  static bool s_toggle = false;

  uint8_t cmd = data->command;
  bool field = (cmd <= RC5_CMD_MASK);

  uint16_t raw = (1U << (RC5_ENCODE_BITS - 1)) | (field ? (1U << (RC5_ENCODE_BITS - 2)) : 0) |
                 (s_toggle ? (1U << (RC5_ENCODE_BITS - 3)) : 0) |
                 ((data->address & RC5_ADDR_MASK) << RC5_ADDR_SHIFT) | (cmd & RC5_CMD_MASK);
  s_toggle = !s_toggle;

  ir_protocol_rc5_menc_t me;
  me_init(&me, symbols);

  for (int i = 0; i < RC5_ENCODE_BITS; i++) {
    int bit = (raw >> (RC5_ENCODE_BITS - 1 - i)) & 1;

    if (bit == 0) {
      me_mark(&me, RC5_UNIT);
      me_space(&me, RC5_UNIT);
    } else {
      if (me.mark_us == 0 && me.space_us == 0 && me.count == 0) {
        me_mark(&me, RC5_UNIT);
      } else {
        me_space(&me, RC5_UNIT);
        me_mark(&me, RC5_UNIT);
      }
    }
  }

  return me_finish(&me);
}