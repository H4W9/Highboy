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

// Pancake (touch-only) input port.
//
// Highboy's UI navigates through an LVGL keypad group (LV_KEY_PREV/NEXT/
// ENTER/ESC/LEFT/RIGHT). Pancake has no buttons, so instead of rewriting
// every screen we keep the keypad indev and drive it from FT6336 touch
// "zones" (Picoware-style): the screen is divided into regions and each
// tap emits one navigation key. One physical tap == one key action
// (rising-edge detection); the finger must lift before the next action.
//
//   ┌──────────────┬──────────────────┬──────────────┐
//   │  BACK (ESC)  │      UP (PREV)    │   UP (PREV)  │   top band
//   ├──────────────┼──────────────────┼──────────────┤
//   │   LEFT       │     OK (ENTER)    │    RIGHT     │   middle band
//   ├──────────────┼──────────────────┼──────────────┤
//   │  DOWN (NEXT) │     DOWN (NEXT)   │   DOWN(NEXT) │   bottom band
//   └──────────────┴──────────────────┴──────────────┘

#include "lv_port_indev.h"

#include "esp_log.h"
#include "core/lv_group.h"

#include "ft6336.h"
#include "st7789.h"  // LCD_H_RES / LCD_V_RES
#include "buzzer.h"
#include "buttons_gpio.h"  // touch zones drive the virtual buttons

static const char *TAG = "LV_PORT_INDEV";

lv_indev_t *indev_keypad = NULL;
lv_group_t *main_group = NULL;

static bool s_is_keyboard_mode = false;

// Zone boundaries (fractions of the display).
#define TOP_BAND_FRAC    0.30f  // y above this fraction = top band
#define BOTTOM_BAND_FRAC 0.70f  // y below this fraction = bottom band
#define LEFT_COL_FRAC    0.28f  // x below this fraction = left column
#define RIGHT_COL_FRAC   0.72f  // x above this fraction = right column

static void keypad_read(lv_indev_t *indev, lv_indev_data_t *data);
static btn_zone_t classify_zone(int16_t x, int16_t y);
static uint32_t zone_to_lvkey(btn_zone_t zone);

void lv_port_indev_init(void) {
  indev_keypad = lv_indev_create();
  lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev_keypad, keypad_read);

  main_group = lv_group_create();
  lv_group_set_default(main_group);
  lv_indev_set_group(indev_keypad, main_group);

  ESP_LOGI(TAG, "Touch-zone input initialized (FT6336 -> keypad navigation)");
}

void lv_port_indev_set_keyboard_mode(bool is_enabled) {
  s_is_keyboard_mode = is_enabled;
}

// Map a touch coordinate to a navigation zone.
static btn_zone_t classify_zone(int16_t x, int16_t y) {
  const int16_t top_y    = (int16_t)(LCD_V_RES * TOP_BAND_FRAC);
  const int16_t bottom_y = (int16_t)(LCD_V_RES * BOTTOM_BAND_FRAC);
  const int16_t left_x   = (int16_t)(LCD_H_RES * LEFT_COL_FRAC);
  const int16_t right_x  = (int16_t)(LCD_H_RES * RIGHT_COL_FRAC);

  if (y < top_y) {
    // Top band: back in the left corner, otherwise "up".
    if (x < left_x) {
      return BTN_ZONE_BACK;
    }
    return BTN_ZONE_UP;
  }

  if (y > bottom_y) {
    return BTN_ZONE_DOWN;
  }

  // Middle band: left / OK / right.
  if (x < left_x) {
    return BTN_ZONE_LEFT;
  }
  if (x > right_x) {
    return BTN_ZONE_RIGHT;
  }
  return BTN_ZONE_OK;
}

// Map a zone to the LVGL keypad key (for screens using the LVGL group).
static uint32_t zone_to_lvkey(btn_zone_t zone) {
  switch (zone) {
    case BTN_ZONE_UP: return s_is_keyboard_mode ? LV_KEY_UP : LV_KEY_PREV;
    case BTN_ZONE_DOWN: return s_is_keyboard_mode ? LV_KEY_DOWN : LV_KEY_NEXT;
    case BTN_ZONE_LEFT: return LV_KEY_LEFT;
    case BTN_ZONE_RIGHT: return LV_KEY_RIGHT;
    case BTN_ZONE_OK: return LV_KEY_ENTER;
    case BTN_ZONE_BACK: return LV_KEY_ESC;
    default: return 0;
  }
}

static void keypad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;

  static uint32_t s_key = 0;
  static bool s_finger_down = false;

  int16_t x = 0, y = 0;
  bool down = ft6336_get_point(&x, &y);

  // Every cycle: publish the currently-touched zone to the virtual buttons so
  // the many screens that poll *_button_is_down() track the finger live.
  btn_zone_t zone = down ? classify_zone(x, y) : BTN_ZONE_NONE;
  buttons_set_active_zone(zone);

  if (down && !s_finger_down) {
    // Rising edge — latch the zone key and report a single press.
    s_finger_down = true;
    s_key = zone_to_lvkey(zone);
    buzzer_click();  // audible tap feedback
    data->key = s_key;
    data->state = LV_INDEV_STATE_PRESSED;
    return;
  }

  if (!down) {
    s_finger_down = false;
  }

  // Hold or idle: report release so each tap is one momentary keypress.
  data->key = s_key;
  data->state = LV_INDEV_STATE_RELEASED;
}
