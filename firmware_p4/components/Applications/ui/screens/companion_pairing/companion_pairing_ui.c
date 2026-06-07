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

#include "companion_pairing_ui.h"

#include <string.h>

#include "core/lv_group.h"
#include "libs/qrcode/lv_qrcode.h"

#include "esp_log.h"

#include "footer_ui.h"
#include "header_ui.h"
#include "host_link_sec.h"
#include "lv_port_indev.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "COMPANION_PAIRING_UI";

#define QR_SIZE          120
#define QR_ALIGN_Y       (-10)
#define TITLE_ALIGN_Y    8
#define HEX_LABEL_WIDTH  220
#define HEX_LABEL_ALIGN_Y 78
#define HINT_ALIGN_Y     (-6)

static lv_obj_t *s_screen = NULL;

static void screen_back_event_cb(lv_event_t *e);

void ui_companion_pairing_open(void) {
  if (s_screen != NULL) {
    lv_obj_del(s_screen);
    s_screen = NULL;
  }

  s_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s_screen, current_theme.screen_base, 0);
  lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

  header_ui_create(s_screen);
  footer_ui_create(s_screen);

  lv_obj_t *title = lv_label_create(s_screen);
  lv_label_set_text(title, "PAIR COMPANION");
  lv_obj_set_style_text_color(title, current_theme.text_main, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, TITLE_ALIGN_Y);

  char psk_hex[HOST_LINK_PSK_HEX_SIZE];
  esp_err_t err = host_link_sec_get_psk_hex(psk_hex, sizeof(psk_hex));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "PSK unavailable: %s", esp_err_to_name(err));
    lv_obj_t *msg = lv_label_create(s_screen);
    lv_label_set_text(msg, "Pairing key unavailable");
    lv_obj_set_style_text_color(msg, current_theme.text_main, 0);
    lv_obj_center(msg);
  } else {
    lv_obj_t *qr = lv_qrcode_create(s_screen);
    lv_qrcode_set_size(qr, QR_SIZE);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_qrcode_update(qr, psk_hex, strlen(psk_hex));
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, QR_ALIGN_Y);
    // Quiet zone so scanners lock on even against a dark theme.
    lv_obj_set_style_border_width(qr, 4, 0);
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);

    lv_obj_t *hex = lv_label_create(s_screen);
    lv_label_set_long_mode(hex, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hex, HEX_LABEL_WIDTH);
    lv_label_set_text(hex, psk_hex);
    lv_obj_set_style_text_color(hex, current_theme.text_main, 0);
    lv_obj_set_style_text_align(hex, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hex, LV_ALIGN_CENTER, 0, HEX_LABEL_ALIGN_Y);
  }

  lv_obj_t *hint = lv_label_create(s_screen);
  lv_label_set_text(hint, "< PRESS TO EXIT >");
  lv_obj_set_style_text_color(hint, current_theme.text_main, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, HINT_ALIGN_Y);

  lv_obj_add_event_cb(s_screen, screen_back_event_cb, LV_EVENT_KEY, NULL);

  if (main_group != NULL) {
    lv_group_add_obj(main_group, s_screen);
    lv_group_focus_obj(s_screen);
  }

  lv_screen_load(s_screen);
}

static void screen_back_event_cb(lv_event_t *e) {
  uint32_t key = lv_event_get_key(e);

  if (key == LV_KEY_ESC || key == LV_KEY_LEFT || key == LV_KEY_ENTER) {
    ui_switch_screen(SCREEN_SETTINGS);
  }
}
