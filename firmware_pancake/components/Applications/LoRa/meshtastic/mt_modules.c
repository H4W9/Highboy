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

#include "mt_modules.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "meshtastic_nodedb.h"
#include "meshtastic_nvs.h"
#include "meshtastic_pki.h"
#include "mt_mod_admin.h"
#include "mt_mod_keyverify.h"
#include "mt_mod_neighborinfo.h"
#include "mt_mod_nodeinfo.h"
#include "mt_mod_position.h"
#include "mt_mod_routing.h"
#include "mt_mod_telemetry.h"
#include "mt_mod_text.h"
#include "mt_mod_traceroute.h"

static const char *TAG = "MT_MODULES";

static uint32_t s_our_node = 0;

static uint64_t dec_varint(const uint8_t *buf, uint16_t max_len, uint16_t *out_consumed) {
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
  if (out_consumed != NULL)
    *out_consumed = i;
  return value;
}

bool mt_parse_data(const uint8_t *data_bytes,
                   uint16_t data_len,
                   uint32_t *out_portnum,
                   const uint8_t **out_payload,
                   uint16_t *out_payload_len,
                   uint32_t *out_request_id) {
  *out_portnum = 0;
  *out_payload = NULL;
  *out_payload_len = 0;
  if (out_request_id != NULL)
    *out_request_id = 0;

  uint16_t i = 0;
  while (i < data_len) {
    uint16_t consumed = 0;
    uint64_t tag = dec_varint(&data_bytes[i], data_len - i, &consumed);
    if (consumed == 0)
      return false;
    i += consumed;

    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 0) {
      uint16_t used = 0;
      uint64_t value = dec_varint(&data_bytes[i], data_len - i, &used);
      i += used;
      if (field == 1)
        *out_portnum = (uint32_t)value;
    } else if (wire_type == 2) {
      uint16_t used = 0;
      uint32_t length = (uint32_t)dec_varint(&data_bytes[i], data_len - i, &used);
      i += used;
      if (i + length > data_len)
        return false;
      if (field == 2) {
        *out_payload = &data_bytes[i];
        *out_payload_len = (uint16_t)length;
      }
      i += length;
    } else if (wire_type == 5) {
      if (i + 4 > data_len)
        return false;
      if (field == 6 && out_request_id != NULL) {
        *out_request_id = ((uint32_t)data_bytes[i]) | ((uint32_t)data_bytes[i + 1] << 8) |
                          ((uint32_t)data_bytes[i + 2] << 16) | ((uint32_t)data_bytes[i + 3] << 24);
      }
      i += 4;
    } else {
      return false;
    }
  }
  return true;
}

esp_err_t mt_modules_init(uint32_t node_num) {
  s_our_node = node_num;

  esp_err_t ret = mt_nvs_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mt_nvs_init failed: %s", esp_err_to_name(ret));
    return ret;
  }

  mt_nodedb_init();
  mt_pki_init();

  mt_mod_admin_init(node_num);
  mt_mod_nodeinfo_init(node_num);
  mt_mod_position_init(node_num);
  mt_mod_text_init(node_num);
  mt_mod_routing_init(node_num);
  mt_mod_traceroute_init(node_num);
  mt_mod_telemetry_init(node_num);
  mt_mod_keyverify_init(node_num);
  mt_mod_neighborinfo_init(node_num);

  ESP_LOGI(TAG,
           "Initialized - 9 modules (Admin, NodeInfo, Position, Text, Routing, TraceRoute, "
           "Telemetry, KeyVerify, NeighborInfo)");
  return ESP_OK;
}

void mt_modules_dispatch(const mt_packet_meta_t *meta,
                         const uint8_t *data_bytes,
                         uint16_t data_len) {
  uint32_t portnum = 0;
  uint32_t request_id = 0;
  const uint8_t *payload = NULL;
  uint16_t payload_len = 0;

  if (!mt_parse_data(data_bytes, data_len, &portnum, &payload, &payload_len, &request_id)) {
    ESP_LOGW(TAG, "Malformed Data proto (%u bytes)", data_len);
    return;
  }

  mt_packet_meta_t m = *meta;
  m.request_id = request_id;

  if (meta->from != 0 && meta->from != s_our_node && portnum != MT_PORT_NODEINFO &&
      mt_nodedb_get(meta->from) == NULL) {
    mt_mod_nodeinfo_request(meta->from);
  }

  switch (portnum) {
    case MT_PORT_TEXT_MESSAGE:
      mt_mod_text_on_received(&m, payload, payload_len, false);
      break;
    case MT_PORT_TEXT_COMPRESSED:
      mt_mod_text_on_received(&m, payload, payload_len, true);
      break;
    case MT_PORT_POSITION:
      mt_mod_position_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_NODEINFO:
      mt_mod_nodeinfo_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_ROUTING:
      mt_mod_routing_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_ADMIN:
      mt_mod_admin_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_TRACEROUTE:
      mt_mod_traceroute_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_TELEMETRY:
      mt_mod_telemetry_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_KEY_VERIFICATION:
      mt_mod_keyverify_on_received(&m, payload, payload_len);
      break;
    case MT_PORT_NEIGHBORINFO:
      mt_mod_neighborinfo_on_received(&m, payload, payload_len);
      break;
    default:
      break;
  }

  if (meta->to == s_our_node && meta->want_ack) {
    mt_mod_routing_maybe_send_ack(&m);
  }
}

void mt_modules_tick(void) {
  uint32_t now_s = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
  mt_mod_nodeinfo_tick(now_s);
  mt_mod_position_tick(now_s);
  mt_mod_telemetry_tick(now_s);
  mt_mod_neighborinfo_tick(now_s);
  mt_mod_admin_tick();
}
