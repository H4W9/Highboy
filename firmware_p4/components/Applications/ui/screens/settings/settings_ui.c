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

#include "settings_ui.h"

#include "esp_log.h"

#include "ui_theme.h"
#include "menu_component_ui.h"
#include "ui_manager.h"
#include "lv_port_indev.h"
#include "buttons_gpio.h"

static const char *TAG = "SETTINGS_UI";

#define NAV_TIMER_PERIOD_MS 50

typedef struct {
  const char *name;
  const char *icon;
  int target;
} settings_item_t;

static const settings_item_t ITEMS[] = {
    {"CONNECTION", "/assets/icons/wifi_menu_icon.bin", SCREEN_CONNECTION_SETTINGS},
    {"INTERFACE", "/assets/icons/interface_menu_icon.bin", SCREEN_INTERFACE_SETTINGS},
    {"DISPLAY", "/assets/icons/display_menu_icon.bin", SCREEN_DISPLAY_SETTINGS},
    {"SOUND", NULL, SCREEN_SOUND_SETTINGS},
    {"BATTERY", "/assets/icons/battery_menu_icon.bin", SCREEN_BATTERY_SETTINGS},
    {"ABOUT", "/assets/icons/about_menu_icon.bin", SCREEN_ABOUT_SETTINGS},
};
#define ITEM_COUNT (sizeof(ITEMS) / sizeof(ITEMS[0]))

static lv_obj_t *s_screen = NULL;
static menu_component_t s_menu;
static lv_timer_t *s_nav_timer = NULL;

static bool s_btn_up_last = false;
static bool s_btn_down_last = false;
static bool s_btn_left_last = false;
static bool s_btn_right_last = false;
static bool s_btn_ok_last = false;
static bool s_btn_back_last = false;

static void nav_timer_cb(lv_timer_t *t);

static void nav_timer_cb(lv_timer_t *t) {
  if (lv_screen_active() != s_screen) {
    lv_timer_delete(t);
    s_nav_timer = NULL;
    return;
  }
  if (ui_input_is_locked())
    return;

  bool up = up_button_is_down();
  bool down = down_button_is_down();
  bool left = left_button_is_down();
  bool right = right_button_is_down();
  bool ok = ok_button_is_down();
  bool back = back_button_is_down();

  if (down && !s_btn_down_last)
    menu_component_next(&s_menu);

  if (up && !s_btn_up_last)
    menu_component_prev(&s_menu);

  if ((back && !s_btn_back_last) || (left && !s_btn_left_last)) {
    ui_switch_screen(SCREEN_MENU);
    return;
  }

  if ((ok && !s_btn_ok_last) || (right && !s_btn_right_last)) {
    int sel = menu_component_get_selected(&s_menu);
    if (sel >= 0 && (size_t)sel < ITEM_COUNT)
      ui_switch_screen(ITEMS[sel].target);
  }

  s_btn_up_last = up;
  s_btn_down_last = down;
  s_btn_left_last = left;
  s_btn_right_last = right;
  s_btn_ok_last = ok;
  s_btn_back_last = back;
}

void ui_settings_open(void) {
  if (s_screen != NULL) {
    lv_obj_del(s_screen);
    s_screen = NULL;
  }

  s_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

  s_menu = menu_component_create(s_screen, "SETTINGS", NULL);

  for (size_t i = 0; i < ITEM_COUNT; i++) {
    menu_component_add_item(&s_menu, ITEMS[i].icon, ITEMS[i].name);
  }

  if (s_nav_timer == NULL)
    s_nav_timer = lv_timer_create(nav_timer_cb, NAV_TIMER_PERIOD_MS, NULL);

  lv_screen_load(s_screen);
}