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

#include "ir_protocol_rc6.h"

#include "esp_log.h"

#include "ir_protocol.h"

typedef struct {
  uint32_t *buf;
  size_t len;
  size_t pos;
  uint32_t units;
  uint32_t unit;
} ir_protocol_rc6_biphase_t;

typedef struct {
  rmt_symbol_word_t *buf;
  size_t count;
  uint32_t mark_us;
  uint32_t space_us;
} ir_protocol_rc6_menc_t;

static const char *TAG = "IR_RC6";

static void bp_init(ir_protocol_rc6_biphase_t *bp, uint32_t *buf, size_t len, uint32_t unit) {
  bp->buf = buf;
  bp->len = len;
  bp->pos = 0;
  bp->unit = unit;
  bp->units = (len > 0) ? (buf[0] + unit / 2) / unit : 0;
}

static int bp_get(ir_protocol_rc6_biphase_t *bp) {
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

static void me_init(ir_protocol_rc6_menc_t *me, rmt_symbol_word_t *buf) {
  me->buf = buf;
  me->count = 0;
  me->mark_us = 0;
  me->space_us = 0;
}

static void me_mark(ir_protocol_rc6_menc_t *me, uint32_t us) {
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

static void me_space(ir_protocol_rc6_menc_t *me, uint32_t us) {
  me->space_us += us;
}

static size_t me_finish(ir_protocol_rc6_menc_t *me) {
  if (me->mark_us > 0 || me->space_us > 0) {
    me->buf[me->count].duration0 = me->mark_us;
    me->buf[me->count].level0 = 1;
    me->buf[me->count].duration1 = me->space_us;
    me->buf[me->count].level1 = 0;
    me->count++;
  }
  return me->count;
}

bool ir_protocol_rc6_decode(const rmt_symbol_word_t *symbols, size_t count, ir_data_t *out_data) {
  if (symbols == NULL || count == 0 || out_data == NULL)
    return false;

  if (!ir_match(symbols[0].duration0, RC6_HEADER_MARK) ||
      !ir_match(symbols[0].duration1, RC6_HEADER_SPACE))
    return false;

  uint32_t flat[RC6_FLAT_BUF_SIZE];
  size_t flat_len = 0;
  for (size_t i = 1; i < count && flat_len < RC6_FLAT_BUF_MAX; i++) {
    if (symbols[i].duration0)
      flat[flat_len++] = symbols[i].duration0;
    if (symbols[i].duration1)
      flat[flat_len++] = symbols[i].duration1;
  }

  ir_protocol_rc6_biphase_t bp;
  bp_init(&bp, flat, flat_len, RC6_UNIT);

  uint32_t raw = 0;
  int decoded = 0;

  for (int i = 0; i < RC6_TOTAL_BITS; i++) {
    int half = (i == RC6_TOGGLE_INDEX) ? 2 : 1;

    int first = -1;
    for (int u = 0; u < half; u++) {
      int l = bp_get(&bp);
      if (l < 0)
        goto done;
      if (u == 0)
        first = l;
    }

    int second = -1;
    for (int u = 0; u < half; u++) {
      int l = bp_get(&bp);
      if (l < 0)
        goto done;
      if (u == 0)
        second = l;
    }

    if (first == 1 && second == 0)
      raw |= (1UL << (RC6_TOTAL_BITS - 1 - i));
    else if (!(first == 0 && second == 1)) {
      ESP_LOGD(TAG, "Invalid biphase at bit %d", i);
      return false;
    }

    decoded++;
  }

done:
  if (decoded < RC6_MIN_DECODED_BITS) {
    ESP_LOGD(TAG, "Too few bits decoded: %d", decoded);
    return false;
  }

  out_data->protocol = IR_PROTO_RC6;
  out_data->address = (raw >> RC6_ADDR_SHIFT) & RC6_ADDR_MASK;
  out_data->command = raw & RC6_CMD_MASK;
  out_data->repeat = false;
  return true;
}

size_t ir_protocol_rc6_encode(const ir_data_t *data, rmt_symbol_word_t *symbols, size_t max) {
  if (data == NULL || symbols == NULL || max == 0)
    return 0;

  if (max < RC6_FLAT_BUF_SIZE) {
    ESP_LOGE(TAG, "Buffer too small for RC6 frame");
    return 0;
  }

  // NOTE: not thread-safe. Must be called from a single task.
  static bool s_toggle = false;

  uint32_t raw = (1UL << RC6_START_BIT_POS) | (s_toggle ? (1UL << RC6_TOGGLE_BIT_POS) : 0) |
                 ((data->address & RC6_ADDR_MASK) << RC6_ADDR_SHIFT) |
                 (data->command & RC6_CMD_MASK);
  s_toggle = !s_toggle;

  symbols[0].duration0 = RC6_HEADER_MARK;
  symbols[0].level0 = 1;
  symbols[0].duration1 = RC6_HEADER_SPACE;
  symbols[0].level1 = 0;

  ir_protocol_rc6_menc_t me;
  me_init(&me, &symbols[1]);

  for (int i = 0; i < RC6_TOTAL_BITS; i++) {
    int bit = (raw >> (RC6_TOTAL_BITS - 1 - i)) & 1;
    uint32_t t = (i == RC6_TOGGLE_INDEX) ? RC6_UNIT * 2 : RC6_UNIT;

    if (bit) {
      me_mark(&me, t);
      me_space(&me, t);
    } else {
      me_space(&me, t);
      me_mark(&me, t);
    }
  }

  return 1 + me_finish(&me);
}