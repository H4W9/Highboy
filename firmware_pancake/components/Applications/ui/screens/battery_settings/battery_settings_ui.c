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

#include "battery_settings_ui.h"

#include "esp_log.h"

#include "buttons_gpio.h"
#include "lv_port_indev.h"
#include "menu_component_ui.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "BATTERY_SETTINGS_UI";

#define NAV_TIMER_INTERVAL_MS 50

#define IDX_PWR_SAVE 0
#define IDX_TIMEOUT  1
#define IDX_MODE     2

static const char *TIMEOUT_OPTIONS[] = {"30s", "1m", "5m", "NEVER"};
#define TIMEOUT_OPTIONS_COUNT (sizeof(TIMEOUT_OPTIONS) / sizeof(TIMEOUT_OPTIONS[0]))

static const char *PERF_OPTIONS[] = {"MIN", "BAL", "MAX"};
#define PERF_OPTIONS_COUNT (sizeof(PERF_OPTIONS) / sizeof(PERF_OPTIONS[0]))

static lv_obj_t *s_screen_battery = NULL;
static menu_component_t s_menu;
static lv_timer_t *s_nav_timer = NULL;
static bool s_is_power_save = false;
static int s_timeout_idx = 1;
static int s_perf_idx = 1;
static bool s_btn_up_last = false;
static bool s_btn_down_last = false;
static bool s_btn_left_last = false;
static bool s_btn_right_last = false;
static bool s_btn_ok_last = false;
static bool s_btn_back_last = false;

static void nav_timer_cb(lv_timer_t *t);

void ui_battery_settings_open(void) {
  if (s_screen_battery != NULL) {
    lv_obj_del(s_screen_battery);
    s_screen_battery = NULL;
  }

  s_screen_battery = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen_battery, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_screen_battery, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_screen_battery, LV_OBJ_FLAG_SCROLLABLE);

  s_menu = menu_component_create(s_screen_battery, "BATTERY", NULL);

  menu_component_add_toggle(
      &s_menu, "/assets/icons/battery_menu_icon.bin", "PWR SAVE", s_is_power_save);
  menu_component_add_selector(&s_menu, NULL, "TIMEOUT", TIMEOUT_OPTIONS[s_timeout_idx]);
  menu_component_add_selector(&s_menu, NULL, "MODE", PERF_OPTIONS[s_perf_idx]);

  if (s_nav_timer == NULL) {
    s_nav_timer = lv_timer_create(nav_timer_cb, NAV_TIMER_INTERVAL_MS, NULL);
  }

  lv_screen_load(s_screen_battery);
}

static void nav_timer_cb(lv_timer_t *t) {
  if (lv_screen_active() != s_screen_battery) {
    lv_timer_delete(t);
    s_nav_timer = NULL;
    return;
  }

  if (ui_input_is_locked()) {
    return;
  }

  bool up = up_button_is_down();
  bool down = down_button_is_down();
  bool left = left_button_is_down();
  bool right = right_button_is_down();
  bool ok = ok_button_is_down();
  bool back = back_button_is_down();

  int sel = menu_component_get_selected(&s_menu);

  if (down && !s_btn_down_last) {
    menu_component_next(&s_menu);
  }

  if (up && !s_btn_up_last) {
    menu_component_prev(&s_menu);
  }

  if (back && !s_btn_back_last) {
    ui_switch_screen(SCREEN_SETTINGS);
  }

  if (right && !s_btn_right_last) {
    if (sel == IDX_PWR_SAVE) {
      menu_component_toggle_item(&s_menu, IDX_PWR_SAVE);
      s_is_power_save = menu_component_get_toggle(&s_menu, IDX_PWR_SAVE);
    } else if (sel == IDX_TIMEOUT) {
      s_timeout_idx = (s_timeout_idx + 1) % (int)TIMEOUT_OPTIONS_COUNT;
      menu_component_set_selector_value(&s_menu, IDX_TIMEOUT, TIMEOUT_OPTIONS[s_timeout_idx]);
    } else if (sel == IDX_MODE) {
      s_perf_idx = (s_perf_idx + 1) % (int)PERF_OPTIONS_COUNT;
      menu_component_set_selector_value(&s_menu, IDX_MODE, PERF_OPTIONS[s_perf_idx]);
    }
  }

  if (left && !s_btn_left_last) {
    if (sel == IDX_PWR_SAVE) {
      menu_component_toggle_item(&s_menu, IDX_PWR_SAVE);
      s_is_power_save = menu_component_get_toggle(&s_menu, IDX_PWR_SAVE);
    } else if (sel == IDX_TIMEOUT) {
      s_timeout_idx = (s_timeout_idx - 1 + (int)TIMEOUT_OPTIONS_COUNT) % (int)TIMEOUT_OPTIONS_COUNT;
      menu_component_set_selector_value(&s_menu, IDX_TIMEOUT, TIMEOUT_OPTIONS[s_timeout_idx]);
    } else if (sel == IDX_MODE) {
      s_perf_idx = (s_perf_idx - 1 + (int)PERF_OPTIONS_COUNT) % (int)PERF_OPTIONS_COUNT;
      menu_component_set_selector_value(&s_menu, IDX_MODE, PERF_OPTIONS[s_perf_idx]);
    }
  }

  s_btn_up_last = up;
  s_btn_down_last = down;
  s_btn_left_last = left;
  s_btn_right_last = right;
  s_btn_ok_last = ok;
  s_btn_back_last = back;
}