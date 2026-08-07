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

#include "mt_mod_routing.h"

#include "esp_log.h"

#include "meshtastic_mesh.h"

static const char *TAG = "MT_MOD_ROUTING";

#define MT_ROUTING_HOP_LIMIT 3

static uint32_t s_node_num = 0;

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

static uint16_t enc_field_varint(uint8_t *buf, uint8_t field_num, uint64_t value) {
  buf[0] = (field_num << 3) | 0;
  return 1 + enc_varint(&buf[1], value);
}

static uint64_t dec_varint(const uint8_t *buf, uint16_t max_len, uint16_t *out_used) {
  uint64_t value = 0;
  uint16_t shift = 0;
  uint16_t i = 0;
  while (i < max_len && i < 10) {
    uint8_t byte = buf[i++];
    value |= ((uint64_t)(byte & 0x7F)) << shift;
    if (!(byte & 0x80))
      break;
    shift += 7;
  }
  if (out_used != NULL)
    *out_used = i;
  return value;
}

static bool parse_routing_error(const uint8_t *payload, uint16_t len, uint32_t *out_error) {
  *out_error = 0;
  uint16_t i = 0;
  while (i < len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&payload[i], len - i, &used);
    if (used == 0)
      return false;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 0) {
      uint16_t vused = 0;
      uint64_t value = dec_varint(&payload[i], len - i, &vused);
      i += vused;
      if (field == 3) {
        *out_error = (uint32_t)value;
        return true;
      }
    } else if (wire_type == 2) {
      uint16_t lused = 0;
      uint32_t length = (uint32_t)dec_varint(&payload[i], len - i, &lused);
      i += lused + length;
    } else if (wire_type == 5) {
      i += 4;
    } else {
      return false;
    }
  }
  return true;
}

void mt_mod_routing_init(uint32_t node_num) {
  s_node_num = node_num;
  ESP_LOGI(TAG, "Initialized");
}

void mt_mod_routing_on_received(const mt_packet_meta_t *meta,
                                const uint8_t *payload,
                                uint16_t len) {
  uint32_t error_code = 0;
  bool parsed = parse_routing_error(payload, len, &error_code);

  if (!parsed) {
    ESP_LOGW(TAG, "Routing from 0x%08lX malformed (%u bytes)", (unsigned long)meta->from, len);
    return;
  }

  uint32_t correlated_id = meta->request_id;

  if (error_code == 0) {
    if (correlated_id != 0 && meshtastic_mesh_retry_ack(correlated_id, false)) {
      ESP_LOGI(TAG,
               "Explicit ACK from 0x%08lX for pkt=0x%08lX",
               (unsigned long)meta->from,
               (unsigned long)correlated_id);
    } else {
      ESP_LOGI(TAG,
               "ACK from 0x%08lX req_id=0x%08lX (no pending retry)",
               (unsigned long)meta->from,
               (unsigned long)correlated_id);
    }
  } else {
    ESP_LOGW(TAG,
             "NAK from 0x%08lX error=%lu req_id=0x%08lX",
             (unsigned long)meta->from,
             (unsigned long)error_code,
             (unsigned long)correlated_id);
    if (correlated_id != 0) {
      meshtastic_mesh_retry_ack(correlated_id, false);
    }
  }
}

void mt_mod_routing_maybe_send_ack(const mt_packet_meta_t *meta) {
  uint8_t routing_payload[4];
  uint16_t rlen = 0;
  rlen += enc_field_varint(&routing_payload[rlen], 3, 0);

  ESP_LOGI(
      TAG, "ACK -> 0x%08lX req_id=0x%08lX", (unsigned long)meta->from, (unsigned long)meta->id);

  meshtastic_mesh_send_data(meta->from,
                            meta->channel,
                            MT_ROUTING_HOP_LIMIT,
                            MT_PORT_ROUTING,
                            routing_payload,
                            rlen,
                            meta->id,
                            false,
                            false);
}
