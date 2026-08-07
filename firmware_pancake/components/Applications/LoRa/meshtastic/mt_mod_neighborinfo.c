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

#include "mt_mod_neighborinfo.h"

#include <string.h>

#include "esp_log.h"

#include "meshtastic_mesh.h"
#include "meshtastic_nodedb.h"

static const char *TAG = "MT_MOD_NEIGHBORINFO";

#define NI_BROADCAST_SECS 14400
#define NI_HOP_LIMIT      2
#define NI_MAX_NEIGHBORS  10
#define NI_BCAST_ADDR     0xFFFFFFFF

static uint32_t s_node_num = 0;
static uint32_t s_last_broadcast_s = 0;

static uint16_t enc_varint(uint8_t *buf, uint64_t value) {
  uint16_t pos = 0;
  do {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (value)
      byte |= 0x80;
    buf[pos++] = byte;
  } while (value);
  return pos;
}

static uint16_t enc_field_varint(uint8_t *buf, uint8_t field, uint64_t v) {
  buf[0] = (field << 3) | 0;
  return 1 + enc_varint(&buf[1], v);
}

static uint16_t enc_field_fixed32_f(uint8_t *buf, uint8_t field, float v) {
  buf[0] = (field << 3) | 5;
  memcpy(&buf[1], &v, 4);
  return 5;
}

static uint16_t enc_field_bytes(uint8_t *buf, uint8_t field, const uint8_t *data, uint16_t len) {
  buf[0] = (field << 3) | 2;
  uint16_t pos = 1 + enc_varint(&buf[1], len);
  memcpy(&buf[pos], data, len);
  return pos + len;
}

static uint16_t build_neighbor_info(uint8_t *out) {
  uint16_t pos = 0;
  pos += enc_field_varint(&out[pos], 1, s_node_num);
  pos += enc_field_varint(&out[pos], 2, 0);
  pos += enc_field_varint(&out[pos], 3, NI_BROADCAST_SECS);

  uint16_t count = mt_nodedb_count();
  if (count > NI_MAX_NEIGHBORS)
    count = NI_MAX_NEIGHBORS;
  for (uint16_t i = 0; i < count; i++) {
    const mt_node_entry_t *e = mt_nodedb_get_by_index(i);
    if (e == NULL || e->num == 0 || e->num == s_node_num)
      continue;
    if (e->hops_away > 0)
      continue;

    uint8_t nb[16];
    uint16_t nl = 0;
    nl += enc_field_varint(&nb[nl], 1, e->num);
    if (e->snr != 0.0f) {
      nl += enc_field_fixed32_f(&nb[nl], 2, e->snr);
    }
    pos += enc_field_bytes(&out[pos], 4, nb, nl);
  }
  return pos;
}

void mt_mod_neighborinfo_init(uint32_t node_num) {
  s_node_num = node_num;
  s_last_broadcast_s = 0;
  ESP_LOGI(
      TAG, "Initialized (interval=%us, max_neighbors=%d)", NI_BROADCAST_SECS, NI_MAX_NEIGHBORS);
}

void mt_mod_neighborinfo_on_received(const mt_packet_meta_t *meta,
                                     const uint8_t *payload,
                                     uint16_t len) {
  (void)payload;
  ESP_LOGI(TAG, "NeighborInfo received from 0x%08lX (%u bytes)", (unsigned long)meta->from, len);
}

void mt_mod_neighborinfo_tick(uint32_t now_s) {
  if (s_last_broadcast_s == 0 && now_s < 60)
    return;
  if (s_last_broadcast_s != 0 && now_s - s_last_broadcast_s < NI_BROADCAST_SECS)
    return;
  s_last_broadcast_s = now_s;

  uint8_t frame[192];
  uint16_t flen = build_neighbor_info(frame);
  ESP_LOGI(TAG, "Broadcast NeighborInfo (%u bytes)", flen);
  meshtastic_mesh_send_data(
      NI_BCAST_ADDR, 0, NI_HOP_LIMIT, MT_PORT_NEIGHBORINFO, frame, flen, 0, false, false);
}
