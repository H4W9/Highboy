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

#include "connect_bt_ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "core/lv_group.h"

#include "bluetooth_service.h"
#include "footer_ui.h"
#include "header_ui.h"
#include "ui_manager.h"
#include "ui_theme.h"

#define BT_MENU_WIDTH         230
#define BT_MENU_HEIGHT        160
#define BT_MENU_OFFSET_Y      5
#define BT_MENU_BORDER_WIDTH  2
#define BT_MENU_PAD           4
#define BT_ITEM_HEIGHT        40
#define BT_ITEM_BORDER_WIDTH  1
#define BT_ITEM_ICON_MARGIN   8
#define BT_ITEM_PAIRED_MARGIN 5
#define BT_SCAN_DELAY_MS      600

extern lv_group_t *main_group;

typedef struct {
  const char *name;
  const char *symbol;
  bool is_paired;
} bt_device_t;

static const bt_device_t MOCK_DEVICES[] = {
    {"PIXEL_BUDS_PRO", LV_SYMBOL_AUDIO, true},
    {"MECHANICAL_KB", LV_SYMBOL_KEYBOARD, true},
    {"UNKNOWN_PHONE", LV_SYMBOL_BLUETOOTH, false},
    {"SMART_WATCH_X", LV_SYMBOL_IMAGE, false},
};
#define MOCK_DEVICES_COUNT (sizeof(MOCK_DEVICES) / sizeof(MOCK_DEVICES[0]))

static lv_obj_t *s_screen_bt_list = NULL;
static lv_style_t s_style_menu;
static lv_style_t s_style_item;
static bool s_is_styles_initialized = false;

static void init_styles(void);
static void bt_item_event_cb(lv_event_t *e);

void ui_connect_bt_open(void) {
  init_styles();

  if (s_screen_bt_list != NULL)
    lv_obj_del(s_screen_bt_list);

  s_screen_bt_list = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen_bt_list, current_theme.screen_base, 0);
  lv_obj_clear_flag(s_screen_bt_list, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(s_screen_bt_list);
  footer_ui_create(s_screen_bt_list);

  lv_obj_t *menu = lv_obj_create(s_screen_bt_list);
  lv_obj_set_size(menu, BT_MENU_WIDTH, BT_MENU_HEIGHT);
  lv_obj_align(menu, LV_ALIGN_CENTER, 0, BT_MENU_OFFSET_Y);
  lv_obj_add_style(menu, &s_style_menu, 0);
  lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *loading_label = lv_label_create(menu);
  lv_label_set_text(loading_label, "BUSCANDO DISPOSITIVOS...");
  lv_obj_set_style_text_color(loading_label, current_theme.text_main, 0);
  lv_obj_set_width(loading_label, lv_pct(100));
  lv_obj_set_style_text_align(loading_label, LV_TEXT_ALIGN_CENTER, 0);

  lv_screen_load(s_screen_bt_list);
  lv_refr_now(NULL);

  vTaskDelay(pdMS_TO_TICKS(BT_SCAN_DELAY_MS));

  lv_obj_del(loading_label);

  for (size_t i = 0; i < MOCK_DEVICES_COUNT; i++) {
    lv_obj_t *item = lv_obj_create(menu);
    lv_obj_set_size(item, lv_pct(100), BT_ITEM_HEIGHT);
    lv_obj_add_style(item, &s_style_item, 0);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(item);
    lv_label_set_text(icon, MOCK_DEVICES[i].symbol);
    lv_obj_set_style_text_color(icon, current_theme.text_main, 0);

    lv_obj_t *lbl_name = lv_label_create(item);
    lv_label_set_text(lbl_name, MOCK_DEVICES[i].name);
    lv_obj_set_style_text_color(lbl_name, current_theme.text_main, 0);
    lv_obj_set_flex_grow(lbl_name, 1);
    lv_obj_set_style_margin_left(lbl_name, BT_ITEM_ICON_MARGIN, 0);

    if (MOCK_DEVICES[i].is_paired) {
      lv_obj_t *paired_icon = lv_label_create(item);
      lv_label_set_text(paired_icon, LV_SYMBOL_OK);
      lv_obj_set_style_text_color(paired_icon, current_theme.text_main, 0);
      lv_obj_set_style_margin_right(paired_icon, BT_ITEM_PAIRED_MARGIN, 0);
    }

    lv_obj_add_event_cb(item, bt_item_event_cb, LV_EVENT_ALL, NULL);

    if (main_group != NULL)
      lv_group_add_obj(main_group, item);
  }
}

static void init_styles(void) {
  if (s_is_styles_initialized)
    return;

  lv_style_init(&s_style_menu);
  lv_style_set_bg_opa(&s_style_menu, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_style_menu, BT_MENU_BORDER_WIDTH);
  lv_style_set_border_color(&s_style_menu, ui_theme_get_accent());
  lv_style_set_radius(&s_style_menu, 0);
  lv_style_set_pad_all(&s_style_menu, BT_MENU_PAD);
  lv_style_set_pad_row(&s_style_menu, BT_MENU_PAD);

  lv_style_init(&s_style_item);
  lv_style_set_bg_color(&s_style_item, current_theme.bg_item_bot);
  lv_style_set_bg_grad_color(&s_style_item, current_theme.bg_item_top);
  lv_style_set_bg_grad_dir(&s_style_item, LV_GRAD_DIR_VER);
  lv_style_set_border_width(&s_style_item, BT_ITEM_BORDER_WIDTH);
  lv_style_set_border_color(&s_style_item, current_theme.border_inactive);
  lv_style_set_radius(&s_style_item, 0);

  s_is_styles_initialized = true;
}

static void bt_item_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *item = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    lv_obj_set_style_border_color(item, ui_theme_get_accent(), 0);
    lv_obj_set_style_border_width(item, BT_MENU_BORDER_WIDTH, 0);
    lv_obj_scroll_to_view(item, LV_ANIM_ON);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_obj_set_style_border_color(item, current_theme.border_inactive, 0);
    lv_obj_set_style_border_width(item, BT_ITEM_BORDER_WIDTH, 0);
  } else if (code == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC || key == LV_KEY_LEFT || key == LV_KEY_ENTER || key == LV_KEY_RIGHT)
      ui_switch_screen(SCREEN_CONNECTION_SETTINGS);
  }
}