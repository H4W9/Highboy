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

#include "rnode_kiss.h"

#include <string.h>

static size_t append_escaped(uint8_t *out, size_t pos, size_t cap, uint8_t b);

void rnode_kiss_decoder_reset(rnode_kiss_decoder_t *d) {
  if (d == NULL) {
    return;
  }
  d->len = 0;
  d->is_in_frame = false;
  d->is_escaped = false;
  d->has_frame_ready = false;
}

bool rnode_kiss_decoder_feed(rnode_kiss_decoder_t *d, uint8_t byte) {
  if (d == NULL) {
    return false;
  }

  if (byte == KISS_FEND) {
    if (d->is_in_frame && d->len > 0) {
      d->has_frame_ready = true;
      d->is_in_frame = false;
      return true;
    }
    d->is_in_frame = true;
    d->is_escaped = false;
    d->len = 0;
    return false;
  }

  if (!d->is_in_frame) {
    return false;
  }

  if (d->is_escaped) {
    d->is_escaped = false;
    uint8_t out = byte;
    if (byte == KISS_TFEND) {
      out = KISS_FEND;
    } else if (byte == KISS_TFESC) {
      out = KISS_FESC;
    }
    if (d->len < RNODE_KISS_FRAME_MAX) {
      d->buf[d->len++] = out;
    } else {
      d->is_in_frame = false;
      d->len = 0;
    }
    return false;
  }

  if (byte == KISS_FESC) {
    d->is_escaped = true;
    return false;
  }

  if (d->len < RNODE_KISS_FRAME_MAX) {
    d->buf[d->len++] = byte;
  } else {
    d->is_in_frame = false;
    d->len = 0;
  }
  return false;
}

bool rnode_kiss_decoder_take_frame(rnode_kiss_decoder_t *d,
                                   uint8_t *out_buf,
                                   size_t cap,
                                   size_t *out_len) {
  if (d == NULL || !d->has_frame_ready) {
    return false;
  }
  size_t n = d->len;
  if (n > cap) {
    n = cap;
  }
  if (out_buf != NULL) {
    memcpy(out_buf, d->buf, n);
  }
  if (out_len != NULL) {
    *out_len = n;
  }
  d->has_frame_ready = false;
  d->len = 0;
  return true;
}

size_t rnode_kiss_encode(
    uint8_t cmd_id, const uint8_t *payload, size_t payload_len, uint8_t *out_buf, size_t cap) {
  if (out_buf == NULL || cap < 3) {
    return 0;
  }
  size_t pos = 0;
  out_buf[pos++] = KISS_FEND;
  size_t after = append_escaped(out_buf, pos, cap, cmd_id);
  if (after == 0) {
    return 0;
  }
  pos = after;
  for (size_t i = 0; i < payload_len; i++) {
    after = append_escaped(out_buf, pos, cap, payload[i]);
    if (after == 0) {
      return 0;
    }
    pos = after;
  }
  if (pos + 1 > cap) {
    return 0;
  }
  out_buf[pos++] = KISS_FEND;
  return pos;
}

static size_t append_escaped(uint8_t *out, size_t pos, size_t cap, uint8_t b) {
  if (b == KISS_FEND) {
    if (pos + 2 > cap) {
      return 0;
    }
    out[pos++] = KISS_FESC;
    out[pos++] = KISS_TFEND;
    return pos;
  }
  if (b == KISS_FESC) {
    if (pos + 2 > cap) {
      return 0;
    }
    out[pos++] = KISS_FESC;
    out[pos++] = KISS_TFESC;
    return pos;
  }
  if (pos + 1 > cap) {
    return 0;
  }
  out[pos++] = b;
  return pos;
}
