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

static const char *TAG = "C5_FLASHER";

#define FLASHER_UART          UART_NUM_1
#define FLASHER_UART_BUF      4096
#define FLASH_BLOCK_SIZE      1024
#define FLASH_BLOCK_HDR_SIZE  16
#define FLASH_CHECKSUM_INIT   0xEF
#define BOOTLOADER_DELAY_MS   50
#define BOOT_RELEASE_DELAY_MS 50
#define FLASH_BEGIN_DELAY_MS  50

// ESP serial protocol (SLIP framing)
#define ESP_ROM_BAUD   115200
#define ESP_FLASH_BAUD 921600
#define SLIP_END       0xC0
#define SLIP_ESC       0xDB
#define SLIP_ESC_END   0xDC
#define SLIP_ESC_ESC   0xDD

// ESP serial protocol commands
#define ESP_CMD_SYNC            0x08
#define ESP_CMD_FLASH_BEGIN     0x02
#define ESP_CMD_FLASH_DATA      0x03
#define ESP_CMD_FLASH_END       0x04
#define ESP_CMD_CHANGE_BAUDRATE 0x0F

#define SYNC_ATTEMPTS        3
#define SYNC_TIMEOUT_MS      100
#define BAUD_TIMEOUT_MS      300
#define ACK_TIMEOUT_MS       500
#define FLASH_BLOCK_DELAY_MS 5

#if C5_FIRMWARE_EMBEDDED
extern const uint8_t c5_firmware_bin_start[] asm("_binary_TentacleOS_C5_bin_start");
extern const uint8_t c5_firmware_bin_end[] asm("_binary_TentacleOS_C5_bin_end");
#endif

typedef struct {
  uint8_t direction;
  uint8_t command;
  uint16_t size;
  uint32_t checksum;
} __attribute__((packed)) c5_flasher_cmd_header_t;

static bool s_ack_supported = false;

static void slip_send_byte(uint8_t b);
static void send_packet(uint8_t cmd, uint8_t *payload, uint16_t len, uint32_t checksum);
static esp_err_t read_response(uint32_t timeout_ms);
static esp_err_t sync_bootloader(void);
static esp_err_t change_baudrate(uint32_t new_baud);
static void wait_for_ack_or_delay(uint32_t fallback_delay_ms);

esp_err_t c5_flasher_init(void) {
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << GPIO_C5_RESET_PIN) | (1ULL << GPIO_C5_BOOT_PIN),
      .pull_down_en = 0,
      .pull_up_en = 1,
  };
  gpio_config(&io_conf);
  gpio_set_level(GPIO_C5_RESET_PIN, 1);
  gpio_set_level(GPIO_C5_BOOT_PIN, 1);

  uart_config_t uart_config = {
      .baud_rate = ESP_ROM_BAUD,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  uart_driver_install(FLASHER_UART, FLASHER_UART_BUF, 0, 0, NULL, 0);
  uart_param_config(FLASHER_UART, &uart_config);
  return uart_set_pin(FLASHER_UART, GPIO_C5_UART_TX_PIN, GPIO_C5_UART_RX_PIN, -1, -1);
}

void c5_flasher_enter_bootloader(void) {
  ESP_LOGI(TAG, "Entering bootloader mode");
  gpio_set_level(GPIO_C5_BOOT_PIN, 0);
  gpio_set_level(GPIO_C5_RESET_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(BOOTLOADER_DELAY_MS));
  gpio_set_level(GPIO_C5_RESET_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(BOOT_RELEASE_DELAY_MS));
  gpio_set_level(GPIO_C5_BOOT_PIN, 1);
}

void c5_flasher_reset_normal(void) {
  gpio_set_level(GPIO_C5_RESET_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(BOOTLOADER_DELAY_MS));
  gpio_set_level(GPIO_C5_RESET_PIN, 1);
  ESP_LOGI(TAG, "C5 reset completed");
}

esp_err_t c5_flasher_update(const uint8_t *bin_data, uint32_t bin_size) {
  if (bin_data == NULL) {
#if C5_FIRMWARE_EMBEDDED
    bin_data = c5_firmware_bin_start;
    bin_size = c5_firmware_bin_end - c5_firmware_bin_start;
#else
    ESP_LOGE(TAG, "Embedded C5 firmware is unavailable");
    return ESP_ERR_NOT_FOUND;
#endif
  }

  if (bin_size == 0) {
    ESP_LOGE(TAG, "Invalid binary size");
    return ESP_ERR_INVALID_ARG;
  }

  c5_flasher_enter_bootloader();

  s_ack_supported = (sync_bootloader() == ESP_OK);
  if (s_ack_supported) {
    change_baudrate(ESP_FLASH_BAUD);
  }

  uint32_t num_blocks = (bin_size + FLASH_BLOCK_SIZE - 1) / FLASH_BLOCK_SIZE;
  uint32_t begin_params[4] = {bin_size, num_blocks, FLASH_BLOCK_SIZE, 0x0000};
  send_packet(ESP_CMD_FLASH_BEGIN, (uint8_t *)begin_params, sizeof(begin_params), 0);
  wait_for_ack_or_delay(FLASH_BEGIN_DELAY_MS);

  for (uint32_t i = 0; i < num_blocks; i++) {
    uint32_t offset = i * FLASH_BLOCK_SIZE;
    uint32_t this_len =
        (bin_size - offset > FLASH_BLOCK_SIZE) ? FLASH_BLOCK_SIZE : bin_size - offset;

    uint8_t block_buffer[FLASH_BLOCK_SIZE + FLASH_BLOCK_HDR_SIZE];
    uint32_t *params = (uint32_t *)block_buffer;
    params[0] = this_len;
    params[1] = i;
    params[2] = 0;
    params[3] = 0;
    memcpy(block_buffer + FLASH_BLOCK_HDR_SIZE, bin_data + offset, this_len);

    uint32_t checksum = FLASH_CHECKSUM_INIT;
    for (uint32_t j = 0; j < this_len; j++) {
      checksum ^= bin_data[offset + j];
    }

    send_packet(ESP_CMD_FLASH_DATA, block_buffer, this_len + FLASH_BLOCK_HDR_SIZE, checksum);
    if ((i % 100) == 0 || i == num_blocks - 1) {
      ESP_LOGI(TAG, "Writing block %lu/%lu", i + 1, num_blocks);
    }
    wait_for_ack_or_delay(FLASH_BLOCK_DELAY_MS);
  }

  uint32_t end_params[1] = {0};
  send_packet(ESP_CMD_FLASH_END, (uint8_t *)end_params, sizeof(end_params), 0);
  wait_for_ack_or_delay(FLASH_BEGIN_DELAY_MS);

  ESP_LOGI(TAG, "Update successful");
  c5_flasher_reset_normal();
  return ESP_OK;
}

static esp_err_t read_response(uint32_t timeout_ms) {
  uint8_t byte;
  bool in_packet = false;
  TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

  while (xTaskGetTickCount() < deadline) {
    if (uart_read_bytes(FLASHER_UART, &byte, 1, pdMS_TO_TICKS(10)) <= 0)
      continue;
    if (byte != SLIP_END)
      continue;
    if (!in_packet) {
      in_packet = true;
    } else {
      return ESP_OK;
    }
  }
  return ESP_ERR_TIMEOUT;
}

static esp_err_t sync_bootloader(void) {
  uint8_t sync_data[36] = {0x07, 0x07, 0x12, 0x20};
  memset(sync_data + 4, 0x55, 32);
  for (int i = 0; i < SYNC_ATTEMPTS; i++) {
    send_packet(ESP_CMD_SYNC, sync_data, sizeof(sync_data), 0);
    if (read_response(SYNC_TIMEOUT_MS) == ESP_OK) {
      ESP_LOGI(TAG, "Bootloader synced — using ACK flow control");
      return ESP_OK;
    }
  }
  ESP_LOGW(TAG, "No ACK from bootloader — falling back to fixed delays");
  return ESP_ERR_TIMEOUT;
}

static esp_err_t change_baudrate(uint32_t new_baud) {
  uint32_t params[2] = {new_baud, 0};
  send_packet(ESP_CMD_CHANGE_BAUDRATE, (uint8_t *)params, sizeof(params), 0);
  if (read_response(BAUD_TIMEOUT_MS) != ESP_OK) {
    ESP_LOGW(TAG, "Baud rate change unconfirmed, staying at %d", ESP_ROM_BAUD);
    return ESP_ERR_TIMEOUT;
  }
  uart_set_baudrate(FLASHER_UART, new_baud);
  vTaskDelay(pdMS_TO_TICKS(50));
  ESP_LOGI(TAG, "Baud rate changed to %lu", (unsigned long)new_baud);
  return ESP_OK;
}

static void wait_for_ack_or_delay(uint32_t fallback_delay_ms) {
  if (s_ack_supported) {
    read_response(ACK_TIMEOUT_MS);
  } else {
    vTaskDelay(pdMS_TO_TICKS(fallback_delay_ms));
  }
}

static void slip_send_byte(uint8_t b) {
  if (b == SLIP_END) {
    uint8_t esc[] = {SLIP_ESC, SLIP_ESC_END};
    uart_write_bytes(FLASHER_UART, esc, 2);
  } else if (b == SLIP_ESC) {
    uint8_t esc[] = {SLIP_ESC, SLIP_ESC_ESC};
    uart_write_bytes(FLASHER_UART, esc, 2);
  } else {
    uart_write_bytes(FLASHER_UART, &b, 1);
  }
}

static void send_packet(uint8_t cmd, uint8_t *payload, uint16_t len, uint32_t checksum) {
  uint8_t start = SLIP_END;
  c5_flasher_cmd_header_t header = {
      .direction = 0x00,
      .command = cmd,
      .size = len,
      .checksum = checksum,
  };

  uart_write_bytes(FLASHER_UART, &start, 1);

  uint8_t *h_ptr = (uint8_t *)&header;
  for (size_t i = 0; i < sizeof(header); i++) {
    slip_send_byte(h_ptr[i]);
  }
  for (uint16_t i = 0; i < len; i++) {
    slip_send_byte(payload[i]);
  }

  uart_write_bytes(FLASHER_UART, &start, 1);
}
