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

#include "c5_flasher.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pin_def.h"
#include "spi_bridge.h"
#include "spi_protocol.h"

static const char *TAG = "C5_FLASHER";

// Live OTA progress, polled by the UI for the progress bar.
static volatile uint32_t s_ota_sent = 0;
static volatile uint32_t s_ota_total = 0;

void c5_flasher_progress(uint32_t *sent, uint32_t *total) {
  if (sent != NULL)
    *sent = s_ota_sent;
  if (total != NULL)
    *total = s_ota_total;
}

// UART1 on the P4 (GPIO_C5_UART_TX_PIN=38 TX / GPIO_C5_UART_RX_PIN=39 RX, defined
// in pin_def.h) is wired to the C5's UART0. The C5 runs an OTA receiver task
// there: we stream the new C5 app image and it writes it to its inactive OTA
// slot and reboots. No ROM download mode involved.
#define OTA_UART     UART_NUM_1
// Must match the C5 OTA receiver. 115200 for reliable bring-up over jumpers.
#define OTA_BAUD     115200
#define OTA_UART_BUF 4096
// Per-block flow control: send a block, wait for the C5 to ACK it (after the
// flash write) before sending the next. Block size must match the C5 receiver.
#define OTA_BLOCK            4096
#define OTA_BLOCK_TIMEOUT_MS 5000
// esp_ota_begin on the C5 erases the partition first; that can take seconds.
#define OTA_BEGIN_TIMEOUT_MS 20000

// Must match firmware_c5/components/Service/ota/ota_service.c.
static const uint8_t OTA_MAGIC[4] = {0xC5, 0xFA, 0x5E, 0x01};
#define OTA_READY 0x52
#define OTA_ACK   0x06
#define OTA_NAK   0x15

#define OTA_SYNC_ATTEMPTS    10
#define OTA_READY_TIMEOUT_MS 1000
#define OTA_ACK_TIMEOUT_MS   30000

// Timeout for the SPI enter-download command (the C5 acks then reboots to ROM).
#define ENTER_DOWNLOAD_TIMEOUT_MS 500

#if C5_FIRMWARE_EMBEDDED
extern const uint8_t c5_app_start[] asm("_binary_TentacleOS_C5_bin_start");
extern const uint8_t c5_app_end[] asm("_binary_TentacleOS_C5_bin_end");
#endif

esp_err_t c5_flasher_init(void) {
  const uart_config_t cfg = {
      .baud_rate = OTA_BAUD,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  if (!uart_is_driver_installed(OTA_UART)) {
    esp_err_t err = uart_driver_install(OTA_UART, OTA_UART_BUF, 0, 0, NULL, 0);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(err));
      return err;
    }
  }
  ESP_ERROR_CHECK(uart_param_config(OTA_UART, &cfg));
  esp_err_t pin_err = uart_set_pin(OTA_UART, GPIO_C5_UART_TX_PIN, GPIO_C5_UART_RX_PIN,
                                   UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (pin_err != ESP_OK) {
    ESP_LOGE(TAG, "uart_set_pin: %s", esp_err_to_name(pin_err));
    return pin_err;
  }
  ESP_LOGI(TAG, "C5 OTA UART ready: UART%d @ %d baud, TX=GPIO%d -> C5 RX, RX=GPIO%d <- C5 TX",
           OTA_UART, OTA_BAUD, GPIO_C5_UART_TX_PIN, GPIO_C5_UART_RX_PIN);
  return ESP_OK;
}

esp_err_t c5_flasher_enter_download(void) {
  ESP_LOGW(TAG, "Requesting C5 to enter ROM download mode over SPI...");
  // The C5 acks then reboots into the ROM stub, so the bridge goes away right
  // after: a timeout here is expected and not an error.
  spi_header_t resp = {0};
  esp_err_t ret = spi_bridge_send_command(SPI_ID_SYSTEM_ENTER_DOWNLOAD, NULL, 0, &resp, NULL,
                                          ENTER_DOWNLOAD_TIMEOUT_MS);
  if (ret == ESP_OK || ret == ESP_ERR_TIMEOUT) {
    // Give the C5 time to reboot into the download stub.
    vTaskDelay(pdMS_TO_TICKS(300));
    return ESP_OK;
  }
  ESP_LOGE(TAG, "enter-download command failed: %s", esp_err_to_name(ret));
  return ret;
}

void c5_flasher_release_uart(void) {
  if (uart_is_driver_installed(OTA_UART))
    uart_driver_delete(OTA_UART);
  // Tri-state both lines so the P4 stops driving the shared GPIO38 (C5 RX) net.
  gpio_reset_pin(GPIO_C5_UART_TX_PIN);
  gpio_reset_pin(GPIO_C5_UART_RX_PIN);
  gpio_set_direction(GPIO_C5_UART_TX_PIN, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_C5_UART_RX_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_C5_UART_TX_PIN, GPIO_FLOATING);
  gpio_set_pull_mode(GPIO_C5_UART_RX_PIN, GPIO_FLOATING);
  ESP_LOGW(TAG, "C5 UART lines released: GPIO%d/GPIO%d now hi-Z inputs.", GPIO_C5_UART_TX_PIN,
           GPIO_C5_UART_RX_PIN);
  ESP_LOGW(TAG, "External USB-serial can now own the C5 UART. Reboot P4 to restore.");
}

esp_err_t c5_flasher_update(const uint8_t *bin_data, uint32_t bin_size) {
#if !C5_FIRMWARE_EMBEDDED
  (void)bin_data;
  (void)bin_size;
  ESP_LOGE(TAG, "Embedded C5 firmware is unavailable");
  return ESP_ERR_NOT_FOUND;
#else
  if (bin_data == NULL) {
    bin_data = c5_app_start;
    bin_size = (uint32_t)(c5_app_end - c5_app_start);
  }
  if (bin_size == 0) {
    ESP_LOGE(TAG, "Invalid image size");
    return ESP_ERR_INVALID_ARG;
  }
  ESP_LOGI(TAG, "C5 OTA: pushing %lu bytes (%s) over UART%d @ %d baud", (unsigned long)bin_size,
           (bin_data == c5_app_start) ? "embedded image" : "caller image", OTA_UART, OTA_BAUD);

  uart_flush(OTA_UART);

  // Handshake: send the magic and wait for the C5 to reply READY before sending
  // the size + image. Retry the magic - if the first byte was lost on the
  // idle->active transition the C5 just won't answer and we send it again.
  bool synced = false;
  for (int attempt = 1; attempt <= OTA_SYNC_ATTEMPTS && !synced; attempt++) {
    uart_flush(OTA_UART);
    uart_write_bytes(OTA_UART, (const char *)OTA_MAGIC, sizeof(OTA_MAGIC));
    uart_wait_tx_done(OTA_UART, pdMS_TO_TICKS(200));
    uint8_t r = 0;
    int n = uart_read_bytes(OTA_UART, &r, 1, pdMS_TO_TICKS(OTA_READY_TIMEOUT_MS));
    if (n == 1 && r == OTA_READY) {
      synced = true;
      ESP_LOGI(TAG, "C5 handshake OK (attempt %d/%d)", attempt, OTA_SYNC_ATTEMPTS);
    } else if (n == 1) {
      ESP_LOGW(TAG, "handshake %d/%d: got 0x%02X, want READY 0x%02X (baud mismatch or line noise?)",
               attempt, OTA_SYNC_ATTEMPTS, r, OTA_READY);
    } else {
      ESP_LOGW(TAG, "handshake %d/%d: no reply within %d ms", attempt, OTA_SYNC_ATTEMPTS,
               OTA_READY_TIMEOUT_MS);
    }
  }
  if (!synced) {
    ESP_LOGE(TAG, "C5 OTA handshake failed after %d attempts", OTA_SYNC_ATTEMPTS);
    ESP_LOGE(TAG, "  the C5 must be RUNNING ITS APP (the OTA receiver on UART0) to answer");
    ESP_LOGE(TAG, "  a blank C5 will NOT reply here -- use ROM flash or passthrough instead");
    ESP_LOGE(TAG, "  also verify wiring TX=GPIO%d/RX=GPIO%d and %d baud on both sides",
             GPIO_C5_UART_TX_PIN, GPIO_C5_UART_RX_PIN, OTA_BAUD);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "C5 synced - sending image");

  // Size header: 4-byte little-endian.
  uint8_t size_hdr[4];
  size_hdr[0] = (uint8_t)(bin_size & 0xFF);
  size_hdr[1] = (uint8_t)((bin_size >> 8) & 0xFF);
  size_hdr[2] = (uint8_t)((bin_size >> 16) & 0xFF);
  size_hdr[3] = (uint8_t)((bin_size >> 24) & 0xFF);
  uart_write_bytes(OTA_UART, (const char *)size_hdr, sizeof(size_hdr));
  uart_wait_tx_done(OTA_UART, pdMS_TO_TICKS(2000));

  // Wait for the C5 to erase the partition (esp_ota_begin) and signal ready
  // before streaming - sending during the erase would lose blocks.
  uint8_t begin = 0;
  int bn = uart_read_bytes(OTA_UART, &begin, 1, pdMS_TO_TICKS(OTA_BEGIN_TIMEOUT_MS));
  if (bn != 1 || begin != OTA_ACK) {
    if (bn != 1)
      ESP_LOGE(TAG,
               "C5 not ready after begin: no reply within %d ms (erase too slow, or C5 hung)",
               OTA_BEGIN_TIMEOUT_MS);
    else
      ESP_LOGE(TAG, "C5 not ready after begin: got 0x%02X, want ACK 0x%02X", begin, OTA_ACK);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "C5 erased its OTA slot - streaming %lu bytes (block=%d)...",
           (unsigned long)bin_size, OTA_BLOCK);
  uint32_t off = 0;
  uint32_t t_stream0 = xTaskGetTickCount();
  s_ota_total = bin_size; // UI progress bar can start tracking now
  s_ota_sent = 0;
  while (off < bin_size) {
    uint32_t chunk = (bin_size - off > OTA_BLOCK) ? OTA_BLOCK : bin_size - off;
    int w = uart_write_bytes(OTA_UART, (const char *)(bin_data + off), chunk);
    if (w < 0) {
      ESP_LOGE(TAG, "uart_write_bytes failed @ %lu", (unsigned long)off);
      return ESP_FAIL;
    }
    uart_wait_tx_done(OTA_UART, pdMS_TO_TICKS(2000));

    // Wait for the C5 to ACK this block before sending the next (flow control).
    uint8_t r = 0;
    int n = uart_read_bytes(OTA_UART, &r, 1, pdMS_TO_TICKS(OTA_BLOCK_TIMEOUT_MS));
    if (n != 1 || r != OTA_ACK) {
      if (n == 1 && r == OTA_NAK) {
        ESP_LOGE(TAG, "C5 NAK at block @ %lu", (unsigned long)off);
      } else {
        ESP_LOGE(TAG, "no block ACK @ %lu (n=%d r=0x%02X)", (unsigned long)off, n, r);
      }
      return ESP_FAIL;
    }

    off += chunk;
    s_ota_sent = off; // feeds the UI progress bar
    if ((off & 0x3FFFF) < OTA_BLOCK || off == bin_size) {
      ESP_LOGI(TAG, "  sent %lu/%lu (%lu%%)", (unsigned long)off, (unsigned long)bin_size,
               (unsigned long)((uint64_t)off * 100 / bin_size));
    }
  }
  uint32_t stream_ms = pdTICKS_TO_MS(xTaskGetTickCount() - t_stream0);
  ESP_LOGI(TAG, "Image sent in %lu ms (%lu B/s) - waiting for C5 to verify and ACK...",
           (unsigned long)stream_ms,
           stream_ms ? (unsigned long)((uint64_t)bin_size * 1000 / stream_ms) : 0UL);

  uint8_t resp = 0;
  int n = uart_read_bytes(OTA_UART, &resp, 1, pdMS_TO_TICKS(OTA_ACK_TIMEOUT_MS));
  if (n == 1 && resp == OTA_ACK) {
    ESP_LOGI(TAG, "C5 ACK - OTA applied, C5 rebooting into new firmware");
    return ESP_OK;
  }
  if (n == 1 && resp == OTA_NAK) {
    ESP_LOGE(TAG, "C5 NAK - OTA rejected (image invalid or transfer error)");
  } else {
    ESP_LOGE(TAG, "no ACK from C5 (n=%d resp=0x%02X)", n, resp);
  }
  return ESP_FAIL;
#endif
}
