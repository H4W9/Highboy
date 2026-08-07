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

#include "ui_badusb_browser.h"

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>

#include "esp_log.h"

#include "footer_ui.h"
#include "header_ui.h"
#include "lv_port_indev.h"
#include "storage_assets.h"
#include "tos_flash_paths.h"
#include "ui_badusb_running.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "UI_BADUSB_BROWSER";

#define BROWSER_LIST_WIDTH        220
#define BROWSER_LIST_HEIGHT       180
#define BROWSER_LIST_BORDER_WIDTH 2

static lv_obj_t *screen_browser = NULL;

static void file_select_event_handler(lv_event_t *e);

void ui_badusb_browser_open(void) {
  if (screen_browser != NULL) {
    lv_obj_del(screen_browser);
  }

  screen_browser = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_browser, current_theme.screen_base, 0);
  lv_obj_remove_flag(screen_browser, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(screen_browser);

  lv_obj_t *list = lv_list_create(screen_browser);
  lv_obj_set_size(list, BROWSER_LIST_WIDTH, BROWSER_LIST_HEIGHT);
  lv_obj_center(list);
  lv_obj_set_style_bg_color(list, current_theme.screen_base, 0);
  lv_obj_set_style_text_color(list, current_theme.text_main, 0);
  lv_obj_set_style_border_color(list, lv_palette_main(LV_PALETTE_DEEP_PURPLE), 0);
  lv_obj_set_style_border_width(list, BROWSER_LIST_BORDER_WIDTH, 0);

  DIR *dir = opendir(FLASH_STORAGE_BADUSB);
  if (dir != NULL) {
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
      if (de->d_type == DT_REG) {
        lv_obj_t *btn = lv_list_add_button(list, LV_SYMBOL_FILE, de->d_name);
        lv_obj_add_event_cb(btn, file_select_event_handler, LV_EVENT_KEY, NULL);
        lv_obj_set_style_bg_color(btn, current_theme.screen_base, 0);
        lv_obj_set_style_text_color(btn, current_theme.text_main, 0);
      }
    }
    closedir(dir);
  } else {
    lv_obj_t *btn = lv_list_add_button(list, LV_SYMBOL_WARNING, "Directory not found");
    lv_obj_set_style_bg_color(btn, current_theme.screen_base, 0);
    lv_obj_set_style_text_color(btn, current_theme.text_main, 0);
  }

  footer_ui_create(screen_browser);

  lv_obj_add_event_cb(screen_browser, file_select_event_handler, LV_EVENT_KEY, NULL);

  if (main_group != NULL) {
    lv_group_add_obj(main_group, list);
    lv_group_focus_obj(list);
  }

  lv_screen_load(screen_browser);
}

static void file_select_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  if (code == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER) {
      const char *filename = lv_list_get_button_text(lv_obj_get_parent(obj), obj);
      ESP_LOGI(TAG, "Selected script: %s", filename);
      ui_badusb_running_set_script(filename);
      ui_switch_screen(SCREEN_BADUSB_LAYOUT);
    } else if (key == LV_KEY_ESC || key == LV_KEY_LEFT) {
      ui_switch_screen(SCREEN_BADUSB_MENU);
    }
  }
}