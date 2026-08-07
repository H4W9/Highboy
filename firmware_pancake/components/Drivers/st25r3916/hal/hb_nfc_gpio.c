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

#include "hb_nfc_gpio.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "hb_nfc_gpio";

#define HB_NFC_GPIO_IRQ_WAIT_US                                      \
  1000 /* Polling interval for hb_nfc_gpio_irq_wait() (1 ms in µs). \
        */

static gpio_num_t s_pin_irq = GPIO_NUM_NC;

esp_err_t hb_nfc_gpio_init(gpio_num_t pin_irq) {
  s_pin_irq = pin_irq;

  gpio_config_t cfg = {
      .pin_bit_mask = 1ULL << pin_irq,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  esp_err_t ret = gpio_config(&cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "IRQ pin %d config fail", pin_irq);
    return ret;
  }

  ESP_LOGI(TAG, "IRQ pin %d OK, level=%d", pin_irq, gpio_get_level(pin_irq));
  return ESP_OK;
}

void hb_nfc_gpio_deinit(void) {
  if (s_pin_irq != GPIO_NUM_NC) {
    gpio_reset_pin(s_pin_irq);
    s_pin_irq = GPIO_NUM_NC;
  }
}

bool hb_nfc_gpio_irq_level(void) {
  if (s_pin_irq == GPIO_NUM_NC) {
    return false;
  }
  return (bool)gpio_get_level(s_pin_irq);
}

bool hb_nfc_gpio_irq_wait(uint32_t timeout_ms) {
  if (s_pin_irq == GPIO_NUM_NC) {
    return false;
  }
  for (uint32_t i = 0; i < timeout_ms; i++) {
    if (gpio_get_level(s_pin_irq) != 0) {
      return true;
    }
    esp_rom_delay_us(HB_NFC_GPIO_IRQ_WAIT_US);
  }
  return false;
}
