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

#include "about_settings_ui.h"

#include "core/lv_group.h"

#include "esp_log.h"

#include "footer_ui.h"
#include "header_ui.h"
#include "lv_port_indev.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "ABOUT_SETTINGS_UI";

#define INFO_BOX_WIDTH          220
#define INFO_BOX_HEIGHT         150
#define INFO_BOX_ALIGN_OFFSET_Y 5
#define INFO_BOX_BORDER_WIDTH   2
#define INFO_BOX_RADIUS         8
#define INFO_BOX_PAD            12
#define TITLE_MARGIN_BOTTOM     10
#define HINT_MARGIN_TOP         15

static lv_obj_t *screen_about = NULL;
static lv_style_t style_info_box;

static void init_styles(void);
static void screen_back_event_cb(lv_event_t *e);

void ui_about_settings_open(void) {
  init_styles();

  if (screen_about != NULL) {
    lv_obj_del(screen_about);
  }

  screen_about = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_about, current_theme.screen_base, 0);
  lv_obj_clear_flag(screen_about, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(screen_about);
  footer_ui_create(screen_about);

  lv_obj_t *info_box = lv_obj_create(screen_about);
  lv_obj_set_size(info_box, INFO_BOX_WIDTH, INFO_BOX_HEIGHT);
  lv_obj_align(info_box, LV_ALIGN_CENTER, 0, INFO_BOX_ALIGN_OFFSET_Y);
  lv_obj_add_style(info_box, &style_info_box, 0);
  lv_obj_set_flex_flow(info_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(info_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(info_box, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(info_box);
  lv_label_set_text(title, "TENTACLE OS");
  lv_obj_set_style_text_color(title, current_theme.text_main, 0);
  lv_obj_set_style_margin_bottom(title, TITLE_MARGIN_BOTTOM, 0);

  lv_obj_t *version = lv_label_create(info_box);
  lv_label_set_text(version, "Version: DEV");
  lv_obj_set_style_text_color(version, current_theme.text_main, 0);

  lv_obj_t *hardware = lv_label_create(info_box);
  lv_label_set_text(hardware, "HW: ESP32-P4");
  lv_obj_set_style_text_color(hardware, current_theme.text_main, 0);

  lv_obj_t *build = lv_label_create(info_box);
  lv_label_set_text(build, "Build: Jan 2026");
  lv_obj_set_style_text_color(build, current_theme.text_main, 0);

  lv_obj_t *hint = lv_label_create(info_box);
  lv_label_set_text(hint, "< PRESS TO EXIT >");
  lv_obj_set_style_text_color(hint, current_theme.text_main, 0);
  lv_obj_set_style_margin_top(hint, HINT_MARGIN_TOP, 0);

  lv_obj_add_event_cb(screen_about, screen_back_event_cb, LV_EVENT_KEY, NULL);

  if (main_group != NULL) {
    lv_group_add_obj(main_group, screen_about);
    lv_group_focus_obj(screen_about);
  }

  lv_screen_load(screen_about);
}

static void init_styles(void) {
  static bool s_styles_initialized = false;

  if (s_styles_initialized) {
    lv_style_reset(&style_info_box);
  }

  lv_style_init(&style_info_box);
  lv_style_set_bg_color(&style_info_box, current_theme.bg_item_bot);
  lv_style_set_bg_grad_color(&style_info_box, current_theme.bg_item_top);
  lv_style_set_bg_grad_dir(&style_info_box, LV_GRAD_DIR_VER);
  lv_style_set_border_width(&style_info_box, INFO_BOX_BORDER_WIDTH);
  lv_style_set_border_color(&style_info_box, ui_theme_get_accent());
  lv_style_set_radius(&style_info_box, INFO_BOX_RADIUS);
  lv_style_set_pad_all(&style_info_box, INFO_BOX_PAD);

  s_styles_initialized = true;
}

static void screen_back_event_cb(lv_event_t *e) {
  uint32_t key = lv_event_get_key(e);

  if (key == LV_KEY_ESC || key == LV_KEY_LEFT || key == LV_KEY_ENTER) {
    ui_switch_screen(SCREEN_SETTINGS);
  }
}