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

/**
 * @file hb_nfc_timer.c
 * @brief HAL timer delay utilities: busy-wait (µs) and scheduler-yield (ms).
 */
#include "hb_nfc_timer.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hb_nfc_timer";

void hb_nfc_timer_delay_us(uint32_t us) {
  esp_rom_delay_us(us);
}

void hb_nfc_timer_delay_ms(uint32_t ms) {
  /* Yield to the scheduler rather than busy-spinning.
   * Minimum 1 ms to guarantee at least one yield even for ms == 0. */
  ESP_LOGD(TAG, "delay %lu ms", (unsigned long)ms);
  vTaskDelay(pdMS_TO_TICKS(ms != 0 ? ms : 1));
}
