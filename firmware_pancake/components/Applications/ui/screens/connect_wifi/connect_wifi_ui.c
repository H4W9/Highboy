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

#include "connect_wifi_ui.h"

#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "lvgl.h"

#include "footer_ui.h"
#include "header_ui.h"
#include "keyboard_ui.h"
#include "msgbox_ui.h"
#include "storage_assets.h"
#include "core/lv_group.h"
#include "ui_manager.h"
#include "ui_theme.h"
#include "wifi_service.h"

#define WIFI_MENU_WIDTH              230
#define WIFI_MENU_HEIGHT             160
#define WIFI_MENU_OFFSET_Y           10
#define WIFI_MENU_BORDER_WIDTH       2
#define WIFI_MENU_PAD                4
#define WIFI_ITEM_HEIGHT             40
#define WIFI_ITEM_BORDER_WIDTH       1
#define WIFI_ITEM_ICON_MARGIN        8
#define WIFI_STATUS_POLL_MAX         20
#define WIFI_STATUS_POLL_INTERVAL_MS 500
#define WIFI_RESTORE_GROUP_DELAY_MS  10
#define WIFI_SSID_MAX_LEN            33
#define WIFI_PASS_MAX_LEN            65

extern lv_group_t *main_group;

static lv_obj_t *s_screen_wifi_list = NULL;
static lv_obj_t *s_wifi_list_cont = NULL;
static lv_style_t s_style_menu;
static lv_style_t s_style_item;
static bool s_is_styles_initialized = false;

static char s_selected_ssid[WIFI_SSID_MAX_LEN];
static char s_selected_pass[WIFI_PASS_MAX_LEN];
static char s_known_pass[WIFI_PASS_MAX_LEN];

static lv_timer_t *s_restore_group_timer = NULL;
static lv_timer_t *s_wifi_status_timer = NULL;
static uint32_t s_wifi_status_poll_count = 0;
static bool s_is_awaiting_connect = false;
static bool s_is_pending_connected = false;

static void init_styles(void);
static bool local_get_known_password(const char *ssid, char *out_password, size_t buffer_size);
static void restore_wifi_group(lv_timer_t *timer);
static void on_msgbox_closed(bool confirm);
static void on_wifi_status_async(void *user_data);
static void wifi_status_timer_cb(lv_timer_t *timer);
static void on_keyboard_submit(const char *text, void *user_data);
static void wifi_item_event_cb(lv_event_t *e);

void ui_connect_wifi_open(void) {
  init_styles();

  if (s_screen_wifi_list != NULL)
    lv_obj_del(s_screen_wifi_list);

  s_screen_wifi_list = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen_wifi_list, current_theme.screen_base, 0);
  lv_obj_clear_flag(s_screen_wifi_list, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(s_screen_wifi_list);
  footer_ui_create(s_screen_wifi_list);

  s_wifi_list_cont = lv_obj_create(s_screen_wifi_list);
  lv_obj_set_size(s_wifi_list_cont, WIFI_MENU_WIDTH, WIFI_MENU_HEIGHT);
  lv_obj_align(s_wifi_list_cont, LV_ALIGN_CENTER, 0, WIFI_MENU_OFFSET_Y);
  lv_obj_add_style(s_wifi_list_cont, &s_style_menu, 0);
  lv_obj_set_flex_flow(s_wifi_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(s_wifi_list_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(s_wifi_list_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(s_wifi_list_cont, LV_DIR_VER);

  lv_obj_t *loading = lv_label_create(s_screen_wifi_list);
  lv_label_set_text(loading, "SCANNING...");
  lv_obj_set_style_text_color(loading, current_theme.text_main, 0);
  lv_obj_center(loading);

  lv_screen_load(s_screen_wifi_list);
  lv_refr_now(NULL);

  if (!wifi_service_is_active()) {
    lv_label_set_text(loading, "WIFI OFF");
    return;
  }

  wifi_service_scan();
  uint16_t ap_count = wifi_service_get_ap_count();
  lv_obj_del(loading);

  if (main_group != NULL)
    lv_group_remove_all_objs(main_group);

  for (uint16_t i = 0; i < ap_count; i++) {
    wifi_ap_record_t *ap = wifi_service_get_ap_record(i);
    if (ap == NULL)
      continue;

    lv_obj_t *item = lv_obj_create(s_wifi_list_cont);
    lv_obj_set_size(item, lv_pct(100), WIFI_ITEM_HEIGHT);
    lv_obj_add_style(item, &s_style_item, 0);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(item);
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(icon, current_theme.text_main, 0);

    lv_obj_t *lbl_ssid = lv_label_create(item);
    lv_label_set_text(lbl_ssid, (char *)ap->ssid);
    lv_obj_set_style_text_color(lbl_ssid, current_theme.text_main, 0);
    lv_obj_set_flex_grow(lbl_ssid, 1);
    lv_obj_set_style_margin_left(lbl_ssid, WIFI_ITEM_ICON_MARGIN, 0);

    if (ap->authmode != WIFI_AUTH_OPEN) {
      lv_obj_t *lock = lv_label_create(item);
      lv_label_set_text(lock, "KEY");
      lv_obj_set_style_text_color(lock, current_theme.text_main, 0);
    }

    lv_obj_set_user_data(item, (void *)ap);
    lv_obj_add_event_cb(item, wifi_item_event_cb, LV_EVENT_ALL, NULL);

    if (main_group != NULL)
      lv_group_add_obj(main_group, item);
  }

  if (main_group != NULL) {
    lv_obj_t *first = lv_obj_get_child(s_wifi_list_cont, 0);
    if (first != NULL)
      lv_group_focus_obj(first);
  }
}

static void init_styles(void) {
  if (s_is_styles_initialized)
    return;

  lv_style_init(&s_style_menu);
  lv_style_set_bg_color(&s_style_menu, current_theme.screen_base);
  lv_style_set_bg_opa(&s_style_menu, LV_OPA_COVER);
  lv_style_set_border_width(&s_style_menu, WIFI_MENU_BORDER_WIDTH);
  lv_style_set_border_color(&s_style_menu, current_theme.border_interface);
  lv_style_set_radius(&s_style_menu, 0);
  lv_style_set_pad_all(&s_style_menu, WIFI_MENU_PAD);

  lv_style_init(&s_style_item);
  lv_style_set_bg_color(&s_style_item, current_theme.bg_item_bot);
  lv_style_set_bg_grad_color(&s_style_item, current_theme.bg_item_top);
  lv_style_set_bg_grad_dir(&s_style_item, LV_GRAD_DIR_VER);
  lv_style_set_border_width(&s_style_item, WIFI_ITEM_BORDER_WIDTH);
  lv_style_set_border_color(&s_style_item, current_theme.border_inactive);
  lv_style_set_radius(&s_style_item, 0);

  s_is_styles_initialized = true;
}

static bool local_get_known_password(const char *ssid, char *out_password, size_t buffer_size) {
  if (ssid == NULL || out_password == NULL)
    return false;

  size_t size = 0;
  char *buffer = (char *)storage_assets_load_file(WIFI_KNOWN_NETWORKS_FILE, &size);
  if (buffer == NULL)
    return false;

  cJSON *root = cJSON_Parse(buffer);
  free(buffer);
  if (root == NULL)
    return false;

  bool found = false;
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, root) {
    cJSON *j_ssid = cJSON_GetObjectItem(item, "ssid");
    if (!cJSON_IsString(j_ssid) || strcmp(j_ssid->valuestring, ssid) != 0)
      continue;

    cJSON *j_pass = cJSON_GetObjectItem(item, "password");
    if (cJSON_IsString(j_pass)) {
      strncpy(out_password, j_pass->valuestring, buffer_size - 1);
      out_password[buffer_size - 1] = '\0';
      found = true;
    }
    break;
  }

  cJSON_Delete(root);
  return found;
}

static void restore_wifi_group(lv_timer_t *timer) {
  (void)timer;
  s_restore_group_timer = NULL;

  if (main_group == NULL || s_wifi_list_cont == NULL)
    return;

  lv_group_remove_all_objs(main_group);
  uint32_t child_count = lv_obj_get_child_cnt(s_wifi_list_cont);
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(s_wifi_list_cont, i);
    if (child != NULL)
      lv_group_add_obj(main_group, child);
  }

  lv_obj_t *first = lv_obj_get_child(s_wifi_list_cont, 0);
  if (first != NULL)
    lv_group_focus_obj(first);
}

static void on_msgbox_closed(bool confirm) {
  (void)confirm;
  if (s_restore_group_timer != NULL)
    lv_timer_del(s_restore_group_timer);

  s_restore_group_timer = lv_timer_create(restore_wifi_group, WIFI_RESTORE_GROUP_DELAY_MS, NULL);
  lv_timer_set_repeat_count(s_restore_group_timer, 1);
}

static void on_wifi_status_async(void *user_data) {
  (void)user_data;
  if (!s_is_awaiting_connect)
    return;

  s_is_awaiting_connect = false;
  msgbox_close();

  if (s_is_pending_connected)
    msgbox_open(LV_SYMBOL_OK, "CONECTADO COM SUCESSO", "OK", NULL, on_msgbox_closed);
  else
    msgbox_open(LV_SYMBOL_CLOSE, "FALHA NA CONEXAO", "OK", NULL, on_msgbox_closed);
}

static void wifi_status_timer_cb(lv_timer_t *timer) {
  if (!s_is_awaiting_connect) {
    lv_timer_del(timer);
    s_wifi_status_timer = NULL;
    return;
  }

  if (wifi_service_is_connected()) {
    s_is_pending_connected = true;
    lv_async_call(on_wifi_status_async, NULL);
    lv_timer_del(timer);
    s_wifi_status_timer = NULL;
    return;
  }

  if (++s_wifi_status_poll_count >= WIFI_STATUS_POLL_MAX) {
    s_is_pending_connected = false;
    lv_async_call(on_wifi_status_async, NULL);
    lv_timer_del(timer);
    s_wifi_status_timer = NULL;
  }
}

static void on_keyboard_submit(const char *text, void *user_data) {
  (void)user_data;
  strncpy(s_selected_pass, text != NULL ? text : "", sizeof(s_selected_pass) - 1);
  s_selected_pass[sizeof(s_selected_pass) - 1] = '\0';

  if (wifi_service_connect_to_ap(s_selected_ssid, s_selected_pass) == ESP_OK) {
    s_is_awaiting_connect = true;
    s_wifi_status_poll_count = 0;
    msgbox_open(LV_SYMBOL_WIFI, "CONECTANDO...", NULL, NULL, NULL);
    s_wifi_status_timer = lv_timer_create(wifi_status_timer_cb, WIFI_STATUS_POLL_INTERVAL_MS, NULL);
  } else {
    msgbox_open(LV_SYMBOL_CLOSE, "FALHA NA CONEXAO", "OK", NULL, on_msgbox_closed);
  }
}

static void wifi_item_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *item = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    lv_obj_set_style_border_color(item, ui_theme_get_accent(), 0);
    lv_obj_set_style_border_width(item, WIFI_MENU_BORDER_WIDTH, 0);
    lv_obj_scroll_to_view(item, LV_ANIM_ON);
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_obj_set_style_border_color(item, current_theme.border_inactive, 0);
    lv_obj_set_style_border_width(item, WIFI_ITEM_BORDER_WIDTH, 0);
  } else if (code == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC || key == LV_KEY_LEFT) {
      ui_switch_screen(SCREEN_CONNECTION_SETTINGS);
    } else if (key == LV_KEY_ENTER || key == LV_KEY_RIGHT) {
      wifi_ap_record_t *ap = (wifi_ap_record_t *)lv_obj_get_user_data(item);
      if (ap == NULL)
        return;

      strncpy(s_selected_ssid, (const char *)ap->ssid, sizeof(s_selected_ssid) - 1);
      s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';

      if (ap->authmode == WIFI_AUTH_OPEN) {
        on_keyboard_submit("", NULL);
      } else if (local_get_known_password(s_selected_ssid, s_known_pass, sizeof(s_known_pass))) {
        on_keyboard_submit(s_known_pass, NULL);
      } else {
        keyboard_open(NULL, on_keyboard_submit, NULL);
      }
    }
  }
}