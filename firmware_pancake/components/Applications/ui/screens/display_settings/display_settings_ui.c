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

#include "display_settings_ui.h"

#include <stdio.h>

#include "esp_log.h"
#include "st7789.h"

#include "buttons_gpio.h"
#include "menu_component_ui.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "DISPLAY_UI";

#define NAV_TIMER_INTERVAL_MS 50
#define BRIGHTNESS_STEP       20
#define BRIGHTNESS_MIN        1
#define ROTATION_BUF_SIZE     8
#define ROTATION_MIN          1
#define ROTATION_MAX          4

typedef enum {
  DISPLAY_ITEM_BRIGHTNESS = 0,
  DISPLAY_ITEM_ROTATION = 1,
} display_item_t;

static lv_obj_t *s_screen_display = NULL;
static menu_component_t s_menu;
static lv_timer_t *s_nav_timer = NULL;

static int s_brightness_val = 3;
static int s_rotation_val = 1;

static bool s_btn_up_last = false;
static bool s_btn_down_last = false;
static bool s_btn_left_last = false;
static bool s_btn_right_last = false;
static bool s_btn_ok_last = false;
static bool s_btn_back_last = false;

static void update_rotation_value(void);
static void nav_timer_cb(lv_timer_t *timer);

void update_lvgl_display_rotation(uint8_t rotation) {
  (void)rotation;
  lv_obj_invalidate(lv_scr_act());
}

void ui_display_settings_open(void) {
  if (s_screen_display != NULL) {
    lv_obj_del(s_screen_display);
    s_screen_display = NULL;
  }

  s_brightness_val = lcd_get_brightness() / BRIGHTNESS_STEP;
  if (s_brightness_val < BRIGHTNESS_MIN)
    s_brightness_val = BRIGHTNESS_MIN;

  s_rotation_val = lcd_get_rotation();

  s_screen_display = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen_display, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_screen_display, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_screen_display, LV_OBJ_FLAG_SCROLLABLE);

  s_menu = menu_component_create(s_screen_display, "DISPLAY", NULL);
  menu_component_add_intensity(
      &s_menu, "/assets/icons/bright_menu_icon.bin", "BRIGHTNESS", s_brightness_val);

  char buf[ROTATION_BUF_SIZE];
  snprintf(buf, sizeof(buf), "%d", s_rotation_val);
  menu_component_add_selector(&s_menu, "/assets/icons/rotate_menu_icon.bin", "ROTATION", buf);

  if (s_nav_timer == NULL)
    s_nav_timer = lv_timer_create(nav_timer_cb, NAV_TIMER_INTERVAL_MS, NULL);

  lv_screen_load(s_screen_display);
}

static void update_rotation_value(void) {
  char buf[ROTATION_BUF_SIZE];
  snprintf(buf, sizeof(buf), "%d", s_rotation_val);
  menu_component_set_selector_value(&s_menu, DISPLAY_ITEM_ROTATION, buf);
}

static void nav_timer_cb(lv_timer_t *timer) {
  if (lv_screen_active() != s_screen_display) {
    lv_timer_delete(timer);
    s_nav_timer = NULL;
    return;
  }

  if (ui_input_is_locked())
    return;

  bool is_up = up_button_is_down();
  bool is_down = down_button_is_down();
  bool is_left = left_button_is_down();
  bool is_right = right_button_is_down();
  bool is_ok = ok_button_is_down();
  bool is_back = back_button_is_down();

  if (is_down && !s_btn_down_last)
    menu_component_next(&s_menu);

  if (is_up && !s_btn_up_last)
    menu_component_prev(&s_menu);

  if (is_back && !s_btn_back_last)
    ui_switch_screen(SCREEN_SETTINGS);

  int sel = menu_component_get_selected(&s_menu);

  if (is_left && !s_btn_left_last) {
    if (sel == DISPLAY_ITEM_BRIGHTNESS) {
      menu_component_intensity_dec(&s_menu, DISPLAY_ITEM_BRIGHTNESS);
      s_brightness_val = menu_component_get_intensity(&s_menu, DISPLAY_ITEM_BRIGHTNESS);
      lcd_set_brightness(s_brightness_val * BRIGHTNESS_STEP);
    } else if (sel == DISPLAY_ITEM_ROTATION) {
      s_rotation_val = (s_rotation_val == ROTATION_MIN) ? ROTATION_MAX : s_rotation_val - 1;
      lcd_set_rotation(s_rotation_val);
      update_lvgl_display_rotation(s_rotation_val);
      update_rotation_value();
    }
  }

  if (is_right && !s_btn_right_last) {
    if (sel == DISPLAY_ITEM_BRIGHTNESS) {
      menu_component_intensity_inc(&s_menu, DISPLAY_ITEM_BRIGHTNESS);
      s_brightness_val = menu_component_get_intensity(&s_menu, DISPLAY_ITEM_BRIGHTNESS);
      lcd_set_brightness(s_brightness_val * BRIGHTNESS_STEP);
    } else if (sel == DISPLAY_ITEM_ROTATION) {
      s_rotation_val = (s_rotation_val % ROTATION_MAX) + 1;
      lcd_set_rotation(s_rotation_val);
      update_lvgl_display_rotation(s_rotation_val);
      update_rotation_value();
    }
  }

  s_btn_up_last = is_up;
  s_btn_down_last = is_down;
  s_btn_left_last = is_left;
  s_btn_right_last = is_right;
  s_btn_ok_last = is_ok;
  s_btn_back_last = is_back;
}