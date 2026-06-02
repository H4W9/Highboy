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

#include "driver/uart.h"
#include "esp_log.h"

#include "esp32_port.h"
#include "esp_loader.h"
#include "pin_def.h"

static const char *TAG = "C5_FLASHER";

#define FLASHER_UART       UART_NUM_1
#define FLASHER_INIT_BAUD  115200
#define FLASHER_FAST_BAUD  921600
#define FLASH_BLOCK_SIZE   1024

// C5 flash layout (matches firmware_c5 partition table / flash_args).
#define C5_BOOTLOADER_OFFSET 0x2000
#define C5_PARTITION_OFFSET  0x8000
#define C5_APP_OFFSET        0x10000

#if C5_FIRMWARE_EMBEDDED
extern const uint8_t c5_bootloader_start[] asm("_binary_bootloader_bin_start");
extern const uint8_t c5_bootloader_end[] asm("_binary_bootloader_bin_end");
extern const uint8_t c5_partition_start[] asm("_binary_partition_table_bin_start");
extern const uint8_t c5_partition_end[] asm("_binary_partition_table_bin_end");
extern const uint8_t c5_app_start[] asm("_binary_TentacleOS_C5_bin_start");
extern const uint8_t c5_app_end[] asm("_binary_TentacleOS_C5_bin_end");
#endif

typedef struct {
  const char *name;
  uint32_t offset;
  const uint8_t *data;
  uint32_t size;
} c5_image_t;

static esp_err_t flash_image(const c5_image_t *img);

esp_err_t c5_flasher_init(void) {
  loader_esp32_config_t config = {
      .baud_rate = FLASHER_INIT_BAUD,
      .uart_port = FLASHER_UART,
      .uart_rx_pin = GPIO_C5_UART_RX_PIN,
      .uart_tx_pin = GPIO_C5_UART_TX_PIN,
      .reset_trigger_pin = GPIO_C5_RESET_PIN,
      .gpio0_trigger_pin = GPIO_C5_BOOT_PIN,
  };

  if (loader_port_esp32_init(&config) != ESP_LOADER_SUCCESS) {
    ESP_LOGE(TAG, "Failed to init serial flasher port");
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t c5_flasher_update(const uint8_t *bin_data, uint32_t bin_size) {
  esp_loader_connect_args_t connect_args = ESP_LOADER_CONNECT_DEFAULT();

  ESP_LOGI(TAG, "Connecting to C5 bootloader");
  if (esp_loader_connect(&connect_args) != ESP_LOADER_SUCCESS) {
    ESP_LOGE(TAG, "Failed to connect to C5");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Connected to target (chip id %d)", esp_loader_get_target());

  // Best-effort speed-up. Only switch the host side if the target accepted it,
  // otherwise stay at the ROM baud rate.
  if (esp_loader_change_transmission_rate(FLASHER_FAST_BAUD) == ESP_LOADER_SUCCESS) {
    if (loader_port_change_transmission_rate(FLASHER_FAST_BAUD) != ESP_LOADER_SUCCESS) {
      ESP_LOGE(TAG, "Host baud switch failed after target switched");
      return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Baud rate raised to %d", FLASHER_FAST_BAUD);
  } else {
    ESP_LOGW(TAG, "Baud change unsupported, staying at %d", FLASHER_INIT_BAUD);
  }

  if (bin_data != NULL) {
    if (bin_size == 0) {
      ESP_LOGE(TAG, "Invalid binary size");
      return ESP_ERR_INVALID_ARG;
    }
    c5_image_t app = {"app", C5_APP_OFFSET, bin_data, bin_size};
    esp_err_t ret = flash_image(&app);
    if (ret != ESP_OK) {
      return ret;
    }
  } else {
#if C5_FIRMWARE_EMBEDDED
    const c5_image_t images[] = {
        {"bootloader", C5_BOOTLOADER_OFFSET, c5_bootloader_start,
         (uint32_t)(c5_bootloader_end - c5_bootloader_start)},
        {"partition-table", C5_PARTITION_OFFSET, c5_partition_start,
         (uint32_t)(c5_partition_end - c5_partition_start)},
        {"app", C5_APP_OFFSET, c5_app_start, (uint32_t)(c5_app_end - c5_app_start)},
    };
    for (size_t i = 0; i < sizeof(images) / sizeof(images[0]); i++) {
      esp_err_t ret = flash_image(&images[i]);
      if (ret != ESP_OK) {
        return ret;
      }
    }
#else
    ESP_LOGE(TAG, "Embedded C5 firmware is unavailable");
    return ESP_ERR_NOT_FOUND;
#endif
  }

  ESP_LOGI(TAG, "Update successful");
  esp_loader_reset_target();
  return ESP_OK;
}

static esp_err_t flash_image(const c5_image_t *img) {
  if (img->size == 0) {
    ESP_LOGE(TAG, "Empty image for %s", img->name);
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG,
           "Flashing %s: %lu bytes @ 0x%05lx",
           img->name,
           (unsigned long)img->size,
           (unsigned long)img->offset);

  if (esp_loader_flash_start(img->offset, img->size, FLASH_BLOCK_SIZE) != ESP_LOADER_SUCCESS) {
    ESP_LOGE(TAG, "flash_start failed for %s", img->name);
    return ESP_FAIL;
  }

  uint8_t block[FLASH_BLOCK_SIZE];
  uint32_t written = 0;
  while (written < img->size) {
    uint32_t chunk = img->size - written;
    if (chunk > FLASH_BLOCK_SIZE) {
      chunk = FLASH_BLOCK_SIZE;
    }
    memcpy(block, img->data + written, chunk);
    if (esp_loader_flash_write(block, chunk) != ESP_LOADER_SUCCESS) {
      ESP_LOGE(TAG, "flash_write failed for %s at offset %lu", img->name, (unsigned long)written);
      return ESP_FAIL;
    }
    written += chunk;
  }

#if MD5_ENABLED
  if (esp_loader_flash_verify() != ESP_LOADER_SUCCESS) {
    ESP_LOGE(TAG, "MD5 verification failed for %s", img->name);
    return ESP_FAIL;
  }
#endif

  return ESP_OK;
}
