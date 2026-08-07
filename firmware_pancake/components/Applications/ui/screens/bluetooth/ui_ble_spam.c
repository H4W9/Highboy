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

#include "ui_ble_spam.h"

#include <stdio.h>

#include "esp_log.h"

#include "canned_spam.h"
#include "footer_ui.h"
#include "header_ui.h"
#include "lv_port_indev.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "UI_BLE_SPAM";

#define SPAM_NAME_MAX_LEN    32
#define TITLE_OFFSET_Y       (-20)
#define INSTR_LABEL_OFFSET_Y 40
#define SPINNER_SIZE         15
#define SPINNER_OFFSET_Y     (-40)

static lv_obj_t *s_screen_spam = NULL;
static char s_current_spam_name[SPAM_NAME_MAX_LEN] = "Unknown";

static void spam_event_cb(lv_event_t *e);

void ui_ble_spam_set_name(const char *name) {
  if (name != NULL) {
    snprintf(s_current_spam_name, sizeof(s_current_spam_name), "%s", name);
  }
}

void ui_ble_spam_open(void) {
  if (s_screen_spam != NULL) {
    lv_obj_del(s_screen_spam);
    s_screen_spam = NULL;
  }

  s_screen_spam = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen_spam, current_theme.screen_base, 0);
  lv_obj_remove_flag(s_screen_spam, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(s_screen_spam);
  footer_ui_create(s_screen_spam);

  lv_obj_t *lbl_title = lv_label_create(s_screen_spam);
  lv_label_set_text_fmt(lbl_title, "SPAM RUNNING:\n#FF0000 %s#", s_current_spam_name);
  lv_label_set_recolor(lbl_title, true);
  lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lbl_title, current_theme.text_main, 0);
  lv_obj_center(lbl_title);
  lv_obj_set_y(lbl_title, TITLE_OFFSET_Y);

  lv_obj_t *lbl_instr = lv_label_create(s_screen_spam);
  lv_label_set_text(lbl_instr, "Press BACK to Stop");
  lv_obj_set_style_text_color(lbl_instr, current_theme.text_main, 0);
  lv_obj_align(lbl_instr, LV_ALIGN_CENTER, 0, INSTR_LABEL_OFFSET_Y);

  lv_obj_t *spinner = lv_spinner_create(s_screen_spam);
  lv_obj_set_size(spinner, SPINNER_SIZE, SPINNER_SIZE);
  lv_obj_align(spinner, LV_ALIGN_BOTTOM_MID, 0, SPINNER_OFFSET_Y);
  lv_obj_set_style_arc_color(spinner, current_theme.border_accent, LV_PART_INDICATOR);

  lv_obj_add_event_cb(s_screen_spam, spam_event_cb, LV_EVENT_KEY, NULL);

  if (main_group != NULL) {
    lv_group_add_obj(main_group, s_screen_spam);
    lv_group_focus_obj(s_screen_spam);
  }

  lv_screen_load(s_screen_spam);
}

static void spam_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC || key == LV_KEY_LEFT) {
      spam_stop();
      ui_switch_screen(SCREEN_BLE_MENU);
    }
  }
}