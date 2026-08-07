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

#include "buzzer.h"

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "pin_def.h"

static const char *TAG = "BUZZER";

// Use a dedicated timer/channel — LEDC_TIMER_0 / CHANNEL_0 belong to the
// display backlight (see st7789.c).
#define BUZ_LEDC_TIMER LEDC_TIMER_1
#define BUZ_LEDC_MODE  LEDC_LOW_SPEED_MODE
#define BUZ_LEDC_CH    LEDC_CHANNEL_1
#define BUZ_LEDC_RES   LEDC_TIMER_10_BIT
#define BUZ_DUTY_50    512  // 50% of 2^10

static bool s_ready = false;
static bool s_enabled = true;

void buzzer_init(void) {
  ledc_timer_config_t timer = {
      .speed_mode = BUZ_LEDC_MODE,
      .timer_num = BUZ_LEDC_TIMER,
      .duty_resolution = BUZ_LEDC_RES,
      .freq_hz = 2000,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  if (ledc_timer_config(&timer) != ESP_OK) {
    ESP_LOGE(TAG, "buzzer timer config failed");
    return;
  }

  ledc_channel_config_t ch = {
      .speed_mode = BUZ_LEDC_MODE,
      .channel = BUZ_LEDC_CH,
      .timer_sel = BUZ_LEDC_TIMER,
      .intr_type = LEDC_INTR_DISABLE,
      .gpio_num = GPIO_BUZZER_PIN,
      .duty = 0,  // start silent
      .hpoint = 0,
  };
  if (ledc_channel_config(&ch) != ESP_OK) {
    ESP_LOGE(TAG, "buzzer channel config failed");
    return;
  }

  s_ready = true;
  ESP_LOGI(TAG, "Passive buzzer ready on GPIO%d", GPIO_BUZZER_PIN);
}

void buzzer_set_enabled(bool enabled) { s_enabled = enabled; }

bool buzzer_is_enabled(void) { return s_enabled; }

void buzzer_tone(uint32_t freq_hz, uint32_t ms) {
  if (!s_ready || !s_enabled || freq_hz == 0) {
    return;
  }
  ledc_set_freq(BUZ_LEDC_MODE, BUZ_LEDC_TIMER, freq_hz);
  ledc_set_duty(BUZ_LEDC_MODE, BUZ_LEDC_CH, BUZ_DUTY_50);
  ledc_update_duty(BUZ_LEDC_MODE, BUZ_LEDC_CH);

  vTaskDelay(pdMS_TO_TICKS(ms));

  ledc_set_duty(BUZ_LEDC_MODE, BUZ_LEDC_CH, 0);
  ledc_update_duty(BUZ_LEDC_MODE, BUZ_LEDC_CH);
}

void buzzer_click(void) { buzzer_tone(4000, 12); }
