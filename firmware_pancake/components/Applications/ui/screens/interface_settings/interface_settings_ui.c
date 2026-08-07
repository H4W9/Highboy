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

#include "interface_settings_ui.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "cJSON.h"

#include "ui_theme.h"
#include "menu_component_ui.h"
#include "ui_manager.h"
#include "lv_port_indev.h"
#include "buttons_gpio.h"
#include "storage_assets.h"
#include "tos_flash_paths.h"

static const char *TAG = "INTERFACE_SETTINGS_UI";

#define INTERFACE_CONFIG_PATH FLASH_CONFIG_INTERFACE
#define HEADER_COUNT          4
#define NAV_TIMER_PERIOD_MS   50
#define CONFIG_FILE_MAX_BYTES 4096
#define CONFIG_DIR_MODE       0777

typedef enum {
  ITEM_THEME = 0,
  ITEM_HEADER = 1,
  ITEM_FOOTER = 2,
  ITEM_COUNT = 3,
} interface_item_t;

static lv_obj_t *s_screen = NULL;
static menu_component_t s_menu;
static lv_timer_t *s_nav_timer = NULL;

static bool s_btn_up_last = false;
static bool s_btn_down_last = false;
static bool s_btn_left_last = false;
static bool s_btn_right_last = false;
static bool s_btn_ok_last = false;
static bool s_btn_back_last = false;

static int s_header_idx = 0;
static bool s_hide_footer = false;
static int s_lang_idx = 0;

static const char *HEADER_OPTIONS[HEADER_COUNT] = {"DEFAULT", "GD TOP", "GD BOT", "MINIMAL"};
static const char *LANG_OPTIONS[] = {"EN", "PT", "ES", "FR"};

static void nav_timer_cb(lv_timer_t *t);

void interface_save_config(void) {
  if (!storage_assets_is_mounted())
    return;

  mkdir("/assets/config", CONFIG_DIR_MODE);
  mkdir(FLASH_MOUNT "/config/screen", CONFIG_DIR_MODE);

  cJSON *root = cJSON_CreateObject();
  if (root == NULL) {
    ESP_LOGE(TAG, "Failed to create JSON object");
    return;
  }

  cJSON_AddNumberToObject(root, "header_idx", s_header_idx);
  cJSON_AddBoolToObject(root, "hide_footer", s_hide_footer);
  cJSON_AddNumberToObject(root, "lang_idx", s_lang_idx);

  char *out = cJSON_PrintUnformatted(root);
  if (out != NULL) {
    FILE *f = fopen(INTERFACE_CONFIG_PATH, "w");
    if (f != NULL) {
      fputs(out, f);
      fclose(f);
    } else {
      ESP_LOGE(TAG, "Failed to open config for writing: %s", INTERFACE_CONFIG_PATH);
    }
    cJSON_free(out);
  } else {
    ESP_LOGE(TAG, "Failed to serialize config JSON");
  }

  cJSON_Delete(root);
}

void interface_load_config(void) {
  if (!storage_assets_is_mounted())
    return;

  FILE *f = fopen(INTERFACE_CONFIG_PATH, "r");
  if (f == NULL)
    return;

  fseek(f, 0, SEEK_END);
  int32_t fsize = (int32_t)ftell(f);
  fseek(f, 0, SEEK_SET);

  if (fsize <= 0 || fsize > CONFIG_FILE_MAX_BYTES) {
    ESP_LOGE(TAG, "Invalid config file size: %ld", (long)fsize);
    fclose(f);
    return;
  }

  char *data = malloc((size_t)fsize + 1);
  if (data == NULL) {
    ESP_LOGE(TAG, "Failed to allocate config read buffer");
    fclose(f);
    return;
  }

  size_t read = fread(data, 1, (size_t)fsize, f);
  fclose(f);

  if ((int32_t)read != fsize) {
    ESP_LOGE(TAG, "Short read on config file: expected %ld, got %zu", (long)fsize, read);
    free(data);
    return;
  }

  data[fsize] = '\0';

  cJSON *root = cJSON_Parse(data);
  free(data);

  if (root == NULL) {
    ESP_LOGE(TAG, "Failed to parse config JSON");
    return;
  }

  cJSON *h = cJSON_GetObjectItem(root, "header_idx");
  cJSON *fsw = cJSON_GetObjectItem(root, "hide_footer");
  cJSON *l = cJSON_GetObjectItem(root, "lang_idx");

  if (cJSON_IsNumber(h))
    s_header_idx = h->valueint;
  if (cJSON_IsBool(fsw))
    s_hide_footer = cJSON_IsTrue(fsw);
  if (cJSON_IsNumber(l))
    s_lang_idx = l->valueint;

  cJSON_Delete(root);
}

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

  if (down && !s_btn_down_last) {
    menu_component_next(&s_menu);
  }
  if (up && !s_btn_up_last) {
    menu_component_prev(&s_menu);
  }

  if (back && !s_btn_back_last) {
    s_btn_back_last = back;
    ui_switch_screen(SCREEN_SETTINGS);
    return;
  }

  if (ok && !s_btn_ok_last) {
    int sel = menu_component_get_selected(&s_menu);
    if (sel == ITEM_THEME) {
      s_btn_ok_last = ok;
      ui_switch_screen(SCREEN_THEME_SELECTOR);
      return;
    }
  }

  if ((left && !s_btn_left_last) || (right && !s_btn_right_last)) {
    int sel = menu_component_get_selected(&s_menu);
    int dir = (right && !s_btn_right_last) ? 1 : -1;

    switch (sel) {
      case ITEM_THEME:
        break;

      case ITEM_HEADER:
        s_header_idx = (s_header_idx + dir + HEADER_COUNT) % HEADER_COUNT;
        menu_component_set_selector_value(&s_menu, ITEM_HEADER, HEADER_OPTIONS[s_header_idx]);
        interface_save_config();
        break;

      case ITEM_FOOTER:
        menu_component_toggle_item(&s_menu, ITEM_FOOTER);
        s_hide_footer = !menu_component_get_toggle(&s_menu, ITEM_FOOTER);
        interface_save_config();
        break;

      default:
        break;
    }
  }

  s_btn_up_last = up;
  s_btn_down_last = down;
  s_btn_left_last = left;
  s_btn_right_last = right;
  s_btn_ok_last = ok;
  s_btn_back_last = back;
}

void ui_interface_settings_open(void) {
  interface_load_config();

  if (s_screen != NULL) {
    lv_obj_del(s_screen);
    s_screen = NULL;
  }

  s_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
  lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

  s_menu = menu_component_create(s_screen, "INTERFACE", NULL);

  menu_component_add_item(&s_menu, "/assets/icons/theme_menu_icon.bin", "THEME");
  menu_component_add_selector(
      &s_menu, "/assets/icons/header_menu_icon.bin", "HEADER", HEADER_OPTIONS[s_header_idx]);
  menu_component_add_toggle(&s_menu, NULL, "FOOTER", !s_hide_footer);

  if (s_nav_timer == NULL) {
    s_nav_timer = lv_timer_create(nav_timer_cb, NAV_TIMER_PERIOD_MS, NULL);
  }

  lv_screen_load(s_screen);
}