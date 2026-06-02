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

#include "spi_bridge.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

#include "bt_dispatcher.h"
#include "bluetooth_service.h"
#include "deauther_detector.h"
#include "session_manager.h"
#include "signal_monitor.h"
#include "spi_slave_driver.h"
#include "wifi_dispatcher.h"
#include "wifi_service.h"
#include "wifi_sniffer.h"

static const char *TAG = "SPI_BRIDGE_C5";

#define SPI_STREAM_QUEUE_LEN  8
#define SPI_BRIDGE_TASK_STACK 4096
#define SPI_BRIDGE_TASK_PRIO  10
#define SPI_IRQ_PULSE_MS      1
#define SPI_RESTART_DELAY_MS  50
#define SPI_FW_VERSION_LEN    32
#define SPI_FW_VERSION_STRING "1.3.0"

typedef struct {
  spi_id_t id;
  uint8_t len;
  uint8_t data[SPI_MAX_PAYLOAD];
} spi_stream_item_t;

static void *s_data_source = NULL;
static uint16_t s_item_count = 0;
static const uint16_t *s_item_count_ptr = NULL;
static uint8_t s_item_size = 0;

static spi_stream_item_t s_stream_queue[SPI_STREAM_QUEUE_LEN];
static uint8_t s_stream_head = 0;
static uint8_t s_stream_tail = 0;
static uint8_t s_stream_count = 0;
static bool s_is_wifi_sniffer_streaming = false;
static bool s_is_bt_sniffer_streaming = false;
static bool s_is_mesh_toradio_streaming = false;
static bool s_is_mcore_rx_streaming = false;
static portMUX_TYPE s_stream_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool s_is_restart_pending = false;
static char s_firmware_version[SPI_FW_VERSION_LEN] = "unknown";

static void load_firmware_version(void);
static bool stream_pop(spi_id_t *out_id, uint8_t *out_data, uint8_t *out_len);
static void bridge_task(void *pvParameters);

// Public functions

void spi_bridge_provide_results(void *source, uint16_t count, uint8_t item_size) {
  s_data_source = source;
  s_item_count = count;
  s_item_count_ptr = NULL;
  s_item_size = item_size;
}

void spi_bridge_provide_results_dynamic(void *source,
                                        const uint16_t *count_ptr,
                                        uint8_t item_size) {
  s_data_source = source;
  s_item_count = 0;
  s_item_count_ptr = count_ptr;
  s_item_size = item_size;
}

bool spi_bridge_stream_is_enabled(spi_id_t id) {
  if (id == SPI_ID_WIFI_APP_SNIFFER)
    return s_is_wifi_sniffer_streaming;
  if (id == SPI_ID_BT_APP_SNIFFER)
    return s_is_bt_sniffer_streaming;
  if (id == SPI_ID_MESH_TORADIO_STREAM)
    return s_is_mesh_toradio_streaming;
  if (id == SPI_ID_MCORE_RX_STREAM)
    return s_is_mcore_rx_streaming;
  return false;
}

void spi_bridge_stream_enable(spi_id_t id, bool enable) {
  if (id == SPI_ID_WIFI_APP_SNIFFER)
    s_is_wifi_sniffer_streaming = enable;
  if (id == SPI_ID_BT_APP_SNIFFER)
    s_is_bt_sniffer_streaming = enable;
  if (id == SPI_ID_MESH_TORADIO_STREAM)
    s_is_mesh_toradio_streaming = enable;
  if (id == SPI_ID_MCORE_RX_STREAM)
    s_is_mcore_rx_streaming = enable;
}

bool spi_bridge_stream_push(spi_id_t id, const uint8_t *data, uint8_t len) {
  if (!spi_bridge_stream_is_enabled(id))
    return false;
  if (data == NULL || len == 0)
    return false;
  if (len > SPI_MAX_PAYLOAD)
    return false;

  portENTER_CRITICAL(&s_stream_mux);
  if (s_stream_count >= SPI_STREAM_QUEUE_LEN) {
    portEXIT_CRITICAL(&s_stream_mux);
    return false;
  }
  spi_stream_item_t *item = &s_stream_queue[s_stream_tail];
  item->id = id;
  item->len = len;
  memcpy(item->data, data, len);
  s_stream_tail = (uint8_t)((s_stream_tail + 1) % SPI_STREAM_QUEUE_LEN);
  s_stream_count++;
  portEXIT_CRITICAL(&s_stream_mux);

  return true;
}

void spi_bridge_notify_master(void) {
  spi_slave_driver_set_irq(1);
  vTaskDelay(pdMS_TO_TICKS(SPI_IRQ_PULSE_MS));
  spi_slave_driver_set_irq(0);
}

esp_err_t spi_bridge_slave_init(void) {
  esp_err_t ret = spi_slave_driver_init();
  if (ret != ESP_OK)
    return ret;
  session_manager_init();
  xTaskCreate(
      bridge_task, "spi_bridge_task", SPI_BRIDGE_TASK_STACK, NULL, SPI_BRIDGE_TASK_PRIO, NULL);
  return ESP_OK;
}

// Static functions

static void load_firmware_version(void) {
  strncpy(s_firmware_version, SPI_FW_VERSION_STRING, sizeof(s_firmware_version) - 1);
  s_firmware_version[sizeof(s_firmware_version) - 1] = '\0';
  ESP_LOGI(TAG, "Firmware version: %s", s_firmware_version);
}

static bool stream_pop(spi_id_t *out_id, uint8_t *out_data, uint8_t *out_len) {
  bool has_item = false;
  portENTER_CRITICAL(&s_stream_mux);
  if (s_stream_count > 0) {
    spi_stream_item_t *item = &s_stream_queue[s_stream_head];
    if (out_id != NULL)
      *out_id = item->id;
    if (out_len != NULL)
      *out_len = item->len;
    if (out_data != NULL && item->len > 0) {
      memcpy(out_data, item->data, item->len);
    }
    s_stream_head = (uint8_t)((s_stream_head + 1) % SPI_STREAM_QUEUE_LEN);
    s_stream_count--;
    has_item = true;
  }
  portEXIT_CRITICAL(&s_stream_mux);
  return has_item;
}

static void bridge_task(void *pvParameters) {
  uint8_t rx_buf[SPI_FRAME_SIZE];
  uint8_t tx_buf[SPI_FRAME_SIZE];
  spi_slave_transaction_t rx_trans;
  spi_slave_transaction_t tx_trans;

  // Keep a receive transaction armed in hardware at all times. The next command
  // RX is re-armed right after the response TX is queued (below), so the master
  // can never clock a command into an unarmed slave — even if this task is
  // preempted between transfers.
  memset(rx_buf, 0, sizeof(rx_buf));
  if (spi_slave_driver_queue(&rx_trans, NULL, rx_buf, SPI_FRAME_SIZE) != ESP_OK) {
    vTaskDelete(NULL);
    return;
  }

  while (1) {
    if (spi_slave_driver_wait() != ESP_OK) {
      memset(rx_buf, 0, sizeof(rx_buf));
      spi_slave_driver_queue(&rx_trans, NULL, rx_buf, SPI_FRAME_SIZE);
      continue;
    }

    spi_header_t *header = (spi_header_t *)rx_buf;
    if (header->sync != SPI_SYNC_BYTE || header->type != SPI_TYPE_CMD ||
        header->length > SPI_MAX_PAYLOAD) {
      memset(rx_buf, 0, sizeof(rx_buf));
      spi_slave_driver_queue(&rx_trans, NULL, rx_buf, SPI_FRAME_SIZE);
      continue;
    }

    spi_status_t status = SPI_STATUS_OK;
    uint8_t resp_payload[SPI_MAX_PAYLOAD];
    uint8_t resp_len = 0;
    bool tx_ready = false; // set when the case already built a complete tx_buf frame

    uint16_t cmd = spi_header_cmd(header);
    const uint8_t *cmd_payload = rx_buf + sizeof(spi_header_t);

    switch (header->category) {
      case SPI_CAT_SYSTEM:
        if (cmd == SPI_ID_SYSTEM_PING) {
          status = SPI_STATUS_OK;
        } else if (cmd == SPI_ID_SYSTEM_REBOOT) {
          status = SPI_STATUS_OK;
          s_is_restart_pending = true;
        } else if (cmd == SPI_ID_SYSTEM_VERSION) {
          if (strcmp(s_firmware_version, "unknown") == 0)
            load_firmware_version();
          size_t ver_len = strlen(s_firmware_version);
          if (ver_len > (SPI_MAX_PAYLOAD - SPI_RESP_STATUS_SIZE))
            ver_len = (SPI_MAX_PAYLOAD - SPI_RESP_STATUS_SIZE);
          memcpy(resp_payload, s_firmware_version, ver_len);
          resp_len = (uint8_t)ver_len;
          status = SPI_STATUS_OK;
        } else if (cmd == SPI_ID_SYSTEM_STATUS) {
          spi_system_status_t sys = {.wifi_active = wifi_service_is_active() ? 1 : 0,
                                     .wifi_connected = wifi_service_is_connected() ? 1 : 0,
                                     .bt_running = bluetooth_service_is_running() ? 1 : 0,
                                     .bt_initialized = bluetooth_service_is_initialized() ? 1 : 0};
          memcpy(resp_payload, &sys, sizeof(sys));
          resp_len = sizeof(sys);
          status = SPI_STATUS_OK;
        } else if (cmd == SPI_ID_SYSTEM_DATA) {
          uint16_t index;
          memcpy(&index, cmd_payload, sizeof(index));
          uint16_t item_count = s_item_count_ptr != NULL ? *s_item_count_ptr : s_item_count;

          if (index == SPI_DATA_INDEX_COUNT) {
            memcpy(resp_payload, &item_count, sizeof(item_count));
            resp_len = sizeof(item_count);
          } else if (index == SPI_DATA_INDEX_STATS) {
            spi_sniffer_stats_t stats = {.packets = wifi_sniffer_get_packet_count(),
                                         .deauths = wifi_sniffer_get_deauth_count(),
                                         .buffer_usage = wifi_sniffer_get_buffer_usage(),
                                         .signal_rssi = signal_monitor_get_rssi(),
                                         .handshake_captured = wifi_sniffer_handshake_captured(),
                                         .pmkid_captured = wifi_sniffer_pmkid_captured()};
            memcpy(resp_payload, &stats, sizeof(stats));
            resp_len = sizeof(stats);
          } else if (index == SPI_DATA_INDEX_DEAUTH_COUNT) {
            uint32_t deauth_count = deauther_detector_get_count();
            memcpy(resp_payload, &deauth_count, sizeof(deauth_count));
            resp_len = sizeof(deauth_count);
          } else if (s_data_source != NULL && index < item_count) {
            memcpy(resp_payload, (uint8_t *)s_data_source + (index * s_item_size), s_item_size);
            resp_len = s_item_size;
          } else {
            status = SPI_STATUS_ERROR;
          }
        } else if (cmd == SPI_ID_SYSTEM_STREAM) {
          spi_id_t stream_id = 0;
          uint8_t stream_len = 0;
          if (stream_pop(&stream_id, resp_payload, &stream_len)) {
            spi_header_t stream_header = {.sync = SPI_SYNC_BYTE,
                                          .type = SPI_TYPE_STREAM,
                                          .category = SPI_CMD_CAT(stream_id),
                                          .op = SPI_CMD_OP(stream_id),
                                          .length = stream_len};
            memset(tx_buf, 0, sizeof(tx_buf));
            memcpy(tx_buf, &stream_header, sizeof(stream_header));
            if (stream_len > 0)
              memcpy(tx_buf + sizeof(stream_header), resp_payload, stream_len);
            tx_ready = true;
          } else {
            status = SPI_STATUS_BUSY;
          }
        } else {
          status = SPI_STATUS_UNSUPPORTED;
        }
        break;
      case SPI_CAT_SESSION:
        if (cmd == SPI_ID_SESSION_HEARTBEAT) {
          spi_heartbeat_req_t req = {0};
          memcpy(&req, cmd_payload, sizeof(req));
          bool alive = session_manager_heartbeat(req.session_id, req.last_acked_seq);
          spi_heartbeat_resp_t resp = {.alive = alive ? (uint8_t)1 : (uint8_t)0};
          memcpy(resp_payload, &resp, sizeof(resp));
          resp_len = sizeof(resp);
          status = SPI_STATUS_OK;
        } else if (cmd == SPI_ID_SESSION_STOP) {
          spi_session_stop_req_t req = {0};
          memcpy(&req, cmd_payload, sizeof(req));
          esp_err_t r = session_manager_stop(req.session_id);
          status = (r == ESP_OK) ? SPI_STATUS_OK : SPI_STATUS_ERROR;
        } else {
          status = SPI_STATUS_UNSUPPORTED;
        }
        break;
      case SPI_CAT_WIFI:
        status = wifi_dispatcher_execute(cmd, cmd_payload, header->length, resp_payload, &resp_len);
        break;
      case SPI_CAT_BT:
      case SPI_CAT_MCORE:
        status = bt_dispatcher_execute(cmd, cmd_payload, header->length, resp_payload, &resp_len);
        break;
      case SPI_CAT_MESH:
        // Meshtastic is split across dispatchers by transport: the WiFi
        // transport ops live in the WiFi dispatcher, the rest (BLE, fromradio,
        // log, status) in the BT dispatcher.
        if (cmd == SPI_ID_MESH_WIFI_INIT || cmd == SPI_ID_MESH_WIFI_STOP) {
          status =
              wifi_dispatcher_execute(cmd, cmd_payload, header->length, resp_payload, &resp_len);
        } else {
          status = bt_dispatcher_execute(cmd, cmd_payload, header->length, resp_payload, &resp_len);
        }
        break;
      default:
        status = SPI_STATUS_UNSUPPORTED;
        break;
    }

    if (!tx_ready) {
      if (resp_len > (SPI_MAX_PAYLOAD - SPI_RESP_STATUS_SIZE)) {
        resp_len = 0;
        status = SPI_STATUS_ERROR;
      }

      spi_header_t resp_header = {.sync = SPI_SYNC_BYTE,
                                  .type = SPI_TYPE_RESP,
                                  .category = header->category,
                                  .op = header->op,
                                  .length = (uint8_t)(resp_len + SPI_RESP_STATUS_SIZE)};
      memset(tx_buf, 0, sizeof(tx_buf));
      memcpy(tx_buf, &resp_header, sizeof(resp_header));
      tx_buf[sizeof(resp_header)] = (uint8_t)status;
      if (resp_len > 0)
        memcpy(tx_buf + sizeof(resp_header) + SPI_RESP_STATUS_SIZE, resp_payload, resp_len);
    }

    // Arm the response, signal the master, then immediately re-arm the next
    // receive so the slave is ready before the response transfer even completes.
    spi_slave_driver_queue(&tx_trans, tx_buf, NULL, SPI_FRAME_SIZE);
    spi_bridge_notify_master();
    memset(rx_buf, 0, sizeof(rx_buf));
    spi_slave_driver_queue(&rx_trans, NULL, rx_buf, SPI_FRAME_SIZE);

    spi_slave_driver_wait(); // wait for the response transfer to complete

    if (s_is_restart_pending) {
      vTaskDelay(pdMS_TO_TICKS(SPI_RESTART_DELAY_MS));
      esp_restart();
    }
  }
}
