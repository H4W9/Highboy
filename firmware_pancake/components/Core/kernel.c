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

// Pancake (ESP32-C5) kernel boot path.
//
// Single-chip, touch-only variant of the P4 kernel. Differences:
//   * No P4<->C5 bridge, CC1101, RFID, or physical buttons.
//   * ST7796S display (via the repurposed st7789 driver) + FT6336 touch.
//   * Passive piezo buzzer on GPIO6.
//   * Phase 1 bring-up: Wi-Fi/BLE radios and console are not started yet.

#include "kernel.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "spi.h"
#include "i2c_init.h"
#include "st7789.h"
#include "ft6336.h"
#include "buzzer.h"
#include "led_control.h"
#include "storage_init.h"
#include "storage_assets.h"
#include "tos_first_boot.h"
#include "tos_config.h"
#include "tos_theme.h"
#include "tos_log.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui_manager.h"
#include "msgbox_ui.h"
#include "sys_monitor.h"

static const char *TAG = "KERNEL";

#define BOOT_SETTLE_MS 1000

void kernel_init(void) {
  // 1. NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // 2. Buses (SPI2 shared by display + SD; I2C for touch)
  spi_init();
  init_i2c();

  // 3. Storage: flash assets partition (fonts/config/theme) is required;
  //    the SD card is optional on Pancake and may be absent — failures here
  //    fall back to defaults.
  storage_init();
  storage_assets_init();
  storage_assets_print_info();

  // 4. Configuration, theme, logging
  tos_first_boot_setup();
  tos_config_load_all();
  tos_log_init();
  tos_theme_load_from_sd();
  ESP_LOGI(TAG, "TentacleOS (Pancake) booted");

  // 5. Peripherals present on Pancake
  led_rgb_init();
  buzzer_init();

  // 6. Touch + Display + LVGL + UI
  ft6336_init();
  st7789_init();  // drives the ST7796S panel on Pancake
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  ui_init();

  // 7. System monitor (Wi-Fi/BLE + console deferred to a later phase)
  sys_monitor_start(false);

  vTaskDelay(pdMS_TO_TICKS(BOOT_SETTLE_MS));
}

// FreeRTOS Safeguards

void safeguard_alert(const char *title, const char *message) {
  ESP_LOGE(TAG, "ALERT: %s - %s", title, message);

  if (ui_acquire()) {
    msgbox_open(LV_SYMBOL_WARNING, message, "OK", NULL, NULL);
    ui_release();
  }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  (void)xTask;
  ESP_LOGE(TAG, "STACK OVERFLOW in task [%s]", pcTaskName);
}

void vApplicationMallocFailedHook(void) {
  ESP_LOGE(TAG, "MALLOC FAILED — out of memory");
}
