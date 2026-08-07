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

// Pancake stub: the ESP32-C5 has no USB-OTG peripheral, so USB HID / Bad USB
// is not available. The screens still render (dummy app); these entry points
// are no-ops. The real TinyUSB-backed implementation lives in the P4 target.

#include "bad_usb.h"

#include "esp_log.h"

#include "hid_hal.h"

static const char *TAG = "BAD_USB";

static void stub_send_key(uint8_t keycode, uint8_t modifiers) {
  (void)keycode;
  (void)modifiers;
}

static void stub_mouse(int8_t x, int8_t y, uint8_t buttons, int8_t wheel) {
  (void)x;
  (void)y;
  (void)buttons;
  (void)wheel;
}

static void stub_wait(void) {}

esp_err_t bad_usb_init(void) {
  ESP_LOGW(TAG, "Bad USB unavailable on Pancake (ESP32-C5 has no USB-OTG)");
  hid_hal_register_callback(stub_send_key, stub_mouse, stub_wait);
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bad_usb_deinit(void) { return ESP_OK; }

void bad_usb_wait_for_connection(void) {}
