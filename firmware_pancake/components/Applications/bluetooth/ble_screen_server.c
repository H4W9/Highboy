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

#include "ble_screen_server.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "spi_bridge.h"
#include "spi_protocol.h"

static const char *TAG = "BLE_SCREEN_SERVER";

#define MAX_CHUNK_SIZE             240
#define COMPRESSION_BUFFER_SIZE    (32 * 1024)
#define COMPRESSION_OVERFLOW_LIMIT 32000
#define COMPRESSION_TAIL_LIMIT     32765
#define RLE_HEADER_MAGIC           0xFE
#define SCREEN_SPI_INIT_TIMEOUT    5000
#define SCREEN_SPI_DEINIT_TIMEOUT  2000
#define SCREEN_SPI_SEND_TIMEOUT    1000
#define SCREEN_SPI_QUERY_TIMEOUT   2000

static uint8_t *s_compression_buffer = NULL;
static bool s_is_initialized = false;

esp_err_t ble_screen_server_init(void) {
  if (s_is_initialized)
    return ESP_OK;

  spi_header_t resp_hdr;
  uint8_t resp_buf[SPI_MAX_PAYLOAD];

  esp_err_t ret = spi_bridge_send_command(
      SPI_ID_BT_SCREEN_INIT, NULL, 0, &resp_hdr, resp_buf, SCREEN_SPI_INIT_TIMEOUT);

  if (ret != ESP_OK || resp_buf[0] != SPI_STATUS_OK) {
    ESP_LOGE(TAG, "Failed to init screen server on C5");
    return ESP_FAIL;
  }

  s_compression_buffer = heap_caps_malloc(COMPRESSION_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (s_compression_buffer == NULL)
    return ESP_ERR_NO_MEM;

  s_is_initialized = true;
  ESP_LOGI(TAG, "Screen Stream Service initialized");
  return ESP_OK;
}

void ble_screen_server_deinit(void) {
  spi_header_t resp_hdr;
  uint8_t resp_buf[SPI_MAX_PAYLOAD];

  spi_bridge_send_command(
      SPI_ID_BT_SCREEN_DEINIT, NULL, 0, &resp_hdr, resp_buf, SCREEN_SPI_DEINIT_TIMEOUT);

  if (s_compression_buffer != NULL) {
    free(s_compression_buffer);
    s_compression_buffer = NULL;
  }
  s_is_initialized = false;
}

void ble_screen_server_send_partial(const uint16_t *px_map, int x, int y, int w, int h) {
  if (!s_is_initialized || s_compression_buffer == NULL || px_map == NULL)
    return;

  size_t comp_idx = 0;

  // Header: [Magic: 0xFE] [X:2] [Y:2] [W:2] [H:2]
  s_compression_buffer[comp_idx++] = RLE_HEADER_MAGIC;
  s_compression_buffer[comp_idx++] = x & 0xFF;
  s_compression_buffer[comp_idx++] = (x >> 8) & 0xFF;
  s_compression_buffer[comp_idx++] = y & 0xFF;
  s_compression_buffer[comp_idx++] = (y >> 8) & 0xFF;
  s_compression_buffer[comp_idx++] = w & 0xFF;
  s_compression_buffer[comp_idx++] = (w >> 8) & 0xFF;
  s_compression_buffer[comp_idx++] = h & 0xFF;
  s_compression_buffer[comp_idx++] = (h >> 8) & 0xFF;

  // RLE Compression
  uint16_t current_pixel = px_map[0];
  uint8_t run_count = 0;
  size_t total_pixels = w * h;

  for (size_t i = 0; i < total_pixels; i++) {
    uint16_t px = px_map[i];

    if (px == current_pixel && run_count < 255) {
      run_count++;
    } else {
      s_compression_buffer[comp_idx++] = run_count;
      s_compression_buffer[comp_idx++] = current_pixel & 0xFF;
      s_compression_buffer[comp_idx++] = (current_pixel >> 8) & 0xFF;

      current_pixel = px;
      run_count = 1;
    }

    if (comp_idx > COMPRESSION_OVERFLOW_LIMIT)
      break;
  }

  if (run_count > 0 && comp_idx < COMPRESSION_TAIL_LIMIT) {
    s_compression_buffer[comp_idx++] = run_count;
    s_compression_buffer[comp_idx++] = current_pixel & 0xFF;
    s_compression_buffer[comp_idx++] = (current_pixel >> 8) & 0xFF;
  }

  // Send compressed data in chunks via SPI
  size_t total_len = comp_idx;
  size_t sent = 0;

  spi_header_t resp_hdr;
  uint8_t resp_buf[SPI_MAX_PAYLOAD];

  while (sent < total_len) {
    size_t chunk_len = total_len - sent;
    if (chunk_len > MAX_CHUNK_SIZE)
      chunk_len = MAX_CHUNK_SIZE;

    spi_bridge_send_command(SPI_ID_BT_SCREEN_SEND_PARTIAL,
                            s_compression_buffer + sent,
                            chunk_len,
                            &resp_hdr,
                            resp_buf,
                            SCREEN_SPI_SEND_TIMEOUT);

    sent += chunk_len;
  }
}

bool ble_screen_server_is_active(void) {
  spi_header_t resp_hdr;
  uint8_t resp_buf[SPI_MAX_PAYLOAD];

  esp_err_t ret = spi_bridge_send_command(
      SPI_ID_BT_SCREEN_IS_ACTIVE, NULL, 0, &resp_hdr, resp_buf, SCREEN_SPI_QUERY_TIMEOUT);

  if (ret != ESP_OK || resp_buf[0] != SPI_STATUS_OK) {
    return false;
  }

  return resp_buf[1];
}
