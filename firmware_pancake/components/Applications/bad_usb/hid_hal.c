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

#include "hid_hal.h"

#include <stddef.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

static const char *TAG = "HID_HAL";

#define KEY_PRESS_DELAY_US   5000
#define KEY_RELEASE_DELAY_US 5000
#define MOUSE_MOVE_DELAY_US  2000
#define MOUSE_CLICK_DELAY_US 5000

static hid_send_cb_t s_send_cb = NULL;
static hid_mouse_cb_t s_mouse_cb = NULL;
static hid_wait_cb_t s_wait_cb = NULL;

void hid_hal_register_callback(hid_send_cb_t send_cb,
                               hid_mouse_cb_t mouse_cb,
                               hid_wait_cb_t wait_cb) {
  s_send_cb = send_cb;
  s_mouse_cb = mouse_cb;
  s_wait_cb = wait_cb;
}

void hid_hal_press_key(uint8_t keycode, uint8_t modifiers) {
  if (s_send_cb == NULL) {
    return;
  }

  s_send_cb(keycode, modifiers);
  ets_delay_us(KEY_PRESS_DELAY_US);

  s_send_cb(0, 0);
  ets_delay_us(KEY_RELEASE_DELAY_US);

  vTaskDelay(0); // Yield to prevent WDT starvation
}

void hid_hal_mouse_move(int8_t x, int8_t y) {
  if (s_mouse_cb == NULL) {
    return;
  }

  s_mouse_cb(x, y, 0, 0);
  ets_delay_us(MOUSE_MOVE_DELAY_US);
  vTaskDelay(0);
}

void hid_hal_mouse_click(uint8_t buttons) {
  if (s_mouse_cb == NULL) {
    return;
  }

  s_mouse_cb(0, 0, buttons, 0); // Press
  ets_delay_us(MOUSE_CLICK_DELAY_US);
  s_mouse_cb(0, 0, 0, 0); // Release
  ets_delay_us(MOUSE_CLICK_DELAY_US);
  vTaskDelay(0);
}

void hid_hal_mouse_scroll(int8_t wheel) {
  if (s_mouse_cb == NULL) {
    return;
  }

  s_mouse_cb(0, 0, 0, wheel);
  ets_delay_us(MOUSE_MOVE_DELAY_US);
  vTaskDelay(0);
}

void hid_hal_wait_for_connection(void) {
  if (s_wait_cb != NULL) {
    s_wait_cb();
  }
}
