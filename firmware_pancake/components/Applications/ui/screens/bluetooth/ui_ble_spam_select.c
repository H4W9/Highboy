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

#include "ui_ble_spam_select.h"

#include "esp_log.h"

#include "canned_spam.h"
#include "font/lv_symbol_def.h"
#include "footer_ui.h"
#include "header_ui.h"
#include "lv_port_indev.h"
#include "ui_ble_spam.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "UI_BLE_SPAM_SELECT";

#define COLOR_BORDER       0x834EC6
#define COLOR_GRADIENT_TOP 0x000000
#define COLOR_GRADIENT_BOT 0x2E0157

#define SCREEN_HEIGHT       240
#define HEADER_HEIGHT       24
#define FOOTER_HEIGHT       20
#define MENU_ALIGN_OFFSET_Y 2
#define MENU_BORDER_WIDTH   2
#define MENU_RADIUS         6
#define MENU_PAD            10
#define BTN_HEIGHT          40
#define BTN_ICON_OFFSET_X   8
#define BTN_BORDER_WIDTH    2
#define BTN_RADIUS          6

static lv_obj_t *s_screen_ble_spam_select = NULL;
static lv_style_t s_style_menu;
static lv_style_t s_style_btn;
static bool s_is_styles_initialized = false;

static void init_styles(void);
static void menu_item_event_cb(lv_event_t *e);
static void spam_toggle_event_cb(lv_event_t *e);
static void ble_spam_select_event_cb(lv_event_t *e);
static void create_menu(lv_obj_t *parent);

void ui_ble_spam_select_open(void) {
  if (s_screen_ble_spam_select != NULL) {
    lv_obj_del(s_screen_ble_spam_select);
    s_screen_ble_spam_select = NULL;
  }

  s_screen_ble_spam_select = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen_ble_spam_select, current_theme.screen_base, 0);
  lv_obj_remove_flag(s_screen_ble_spam_select, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(s_screen_ble_spam_select);
  footer_ui_create(s_screen_ble_spam_select);
  create_menu(s_screen_ble_spam_select);

  lv_obj_add_event_cb(s_screen_ble_spam_select, ble_spam_select_event_cb, LV_EVENT_KEY, NULL);

  lv_screen_load(s_screen_ble_spam_select);
}

static void init_styles(void) {
  if (s_is_styles_initialized) {
    return;
  }

  lv_style_init(&s_style_menu);
  lv_style_set_bg_opa(&s_style_menu, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_style_menu, MENU_BORDER_WIDTH);
  lv_style_set_border_color(&s_style_menu, lv_color_hex(COLOR_BORDER));
  lv_style_set_radius(&s_style_menu, MENU_RADIUS);
  lv_style_set_pad_all(&s_style_menu, MENU_PAD);
  lv_style_set_pad_row(&s_style_menu, MENU_PAD);

  lv_style_init(&s_style_btn);
  lv_style_set_bg_color(&s_style_btn, lv_color_hex(COLOR_GRADIENT_BOT));
  lv_style_set_bg_grad_color(&s_style_btn, lv_color_hex(COLOR_GRADIENT_TOP));
  lv_style_set_bg_grad_dir(&s_style_btn, LV_GRAD_DIR_VER);
  lv_style_set_border_width(&s_style_btn, BTN_BORDER_WIDTH);
  lv_style_set_border_color(&s_style_btn, lv_color_hex(COLOR_BORDER));
  lv_style_set_radius(&s_style_btn, BTN_RADIUS);

  s_is_styles_initialized = true;
}

static void menu_item_event_cb(lv_event_t *e) {
  lv_obj_t *img_sel = lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_FOCUSED) {
    lv_obj_clear_flag(img_sel, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_obj_add_flag(img_sel, LV_OBJ_FLAG_HIDDEN);
  }
}

static void spam_toggle_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) {
    return;
  }

  uint32_t key = lv_event_get_key(e);
  if (key == LV_KEY_ENTER) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);

    const canned_spam_type_t *type = spam_get_attack_type(index);
    if (type != NULL) {
      ui_ble_spam_set_name(type->name);
    }

    ESP_LOGI(TAG, "Starting Spam Index: %d", index);
    spam_start(index);

    ui_switch_screen(SCREEN_BLE_SPAM);
  }
}

static void ble_spam_select_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_KEY) {
    if (lv_event_get_key(e) == LV_KEY_ESC) {
      ESP_LOGI(TAG, "Returning to BLE Options Menu");
      ui_switch_screen(SCREEN_BLE_SPAM_SELECT);
    }
  }
}

static void create_menu(lv_obj_t *parent) {
  init_styles();

  lv_coord_t menu_h = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;

  lv_obj_t *menu = lv_obj_create(parent);
  lv_obj_set_size(menu, SCREEN_HEIGHT, menu_h);
  lv_obj_align(menu, LV_ALIGN_CENTER, 0, MENU_ALIGN_OFFSET_Y);
  lv_obj_add_style(menu, &s_style_menu, 0);
  lv_obj_set_scroll_dir(menu, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);

  static const void *s_ble_icon = NULL;
  static const void *s_select_icon = NULL;

  if (s_ble_icon == NULL) {
    s_ble_icon = LV_SYMBOL_BLUETOOTH;
  }
  if (s_select_icon == NULL) {
    s_select_icon = LV_SYMBOL_RIGHT;
  }

  int count = spam_get_attack_count();

  for (int i = 0; i < count; i++) {
    const canned_spam_type_t *type = spam_get_attack_type(i);
    if (type == NULL) {
      continue;
    }

    lv_obj_t *btn = lv_btn_create(menu);
    lv_obj_set_size(btn, lv_pct(100), BTN_HEIGHT);
    lv_obj_add_style(btn, &s_style_btn, 0);
    lv_obj_set_style_anim_time(btn, 0, 0);

    lv_obj_t *img_left = lv_label_create(btn);
    lv_label_set_text(img_left, s_ble_icon);
    lv_obj_align(img_left, LV_ALIGN_LEFT_MID, BTN_ICON_OFFSET_X, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, type->name);
    lv_obj_center(lbl);

    lv_obj_t *img_sel = lv_label_create(btn);
    lv_label_set_text(img_sel, s_select_icon);
    lv_obj_align(img_sel, LV_ALIGN_RIGHT_MID, -BTN_ICON_OFFSET_X, 0);
    lv_obj_add_flag(img_sel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(btn, menu_item_event_cb, LV_EVENT_FOCUSED, img_sel);
    lv_obj_add_event_cb(btn, menu_item_event_cb, LV_EVENT_DEFOCUSED, img_sel);
    lv_obj_add_event_cb(btn, spam_toggle_event_cb, LV_EVENT_KEY, (void *)(intptr_t)i);
    lv_obj_add_event_cb(btn, ble_spam_select_event_cb, LV_EVENT_KEY, NULL);

    if (main_group != NULL) {
      lv_group_add_obj(main_group, btn);
    }
  }
}