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

#include "buttons_gpio.h"
#include "footer_ui.h"
#include "header_ui.h"
#include "host_link_ble.h"
#include "host_link_sec.h"
#include "lv_port_indev.h"
#include "toggle_ui.h"
#include "ui_manager.h"
#include "ui_theme.h"

static const char *TAG = "COMPANION_PAIRING_UI";

#define QR_SIZE             104
#define QR_ALIGN_Y          26
#define TITLE_ALIGN_Y       6
#define HEX_LABEL_WIDTH     220
#define HEX_LABEL_ALIGN_Y   84
#define STATUS_ALIGN_Y      (-58)
#define ADV_ROW_ALIGN_Y     (-30)
#define HINT_ALIGN_Y        (-6)
#define NAV_TIMER_PERIOD_MS 50

static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_status = NULL;
static toggle_ui_t s_adv_toggle;
static lv_timer_t *s_nav_timer = NULL;

static bool s_btn_ok_last = false;
static bool s_btn_back_last = false;

static void nav_timer_cb(lv_timer_t *t);
static void refresh_status(void);

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
  lv_label_set_text(title, "COMPANION APP");
  lv_obj_set_style_text_color(title, current_theme.text_main, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, TITLE_ALIGN_Y);

  char psk_hex[HOST_LINK_PSK_HEX_SIZE];
  esp_err_t err = host_link_sec_get_psk_hex(psk_hex, sizeof(psk_hex));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "PSK unavailable: %s", esp_err_to_name(err));
    lv_obj_t *msg = lv_label_create(s_screen);
    lv_label_set_text(msg, "Pairing key unavailable");
    lv_obj_set_style_text_color(msg, current_theme.text_main, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, QR_ALIGN_Y);
  } else {
    lv_obj_t *qr = lv_qrcode_create(s_screen);
    lv_qrcode_set_size(qr, QR_SIZE);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_qrcode_update(qr, psk_hex, strlen(psk_hex));
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, QR_ALIGN_Y);
    lv_obj_set_style_border_width(qr, 4, 0);
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);

    lv_obj_t *hex = lv_label_create(s_screen);
    lv_label_set_long_mode(hex, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hex, HEX_LABEL_WIDTH);
    lv_label_set_text(hex, psk_hex);
    lv_obj_set_style_text_color(hex, current_theme.text_main, 0);
    lv_obj_set_style_text_align(hex, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hex, LV_ALIGN_TOP_MID, 0, HEX_LABEL_ALIGN_Y);
  }

  // Status line (advertising + connection), refreshed by the nav timer.
  s_status = lv_label_create(s_screen);
  lv_obj_set_style_text_color(s_status, current_theme.text_main, 0);
  lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, STATUS_ALIGN_Y);

  // Advertising on/off row: label + toggle switch.
  lv_obj_t *adv_label = lv_label_create(s_screen);
  lv_label_set_text(adv_label, "Advertising");
  lv_obj_set_style_text_color(adv_label, current_theme.text_main, 0);
  lv_obj_align(adv_label, LV_ALIGN_BOTTOM_LEFT, 18, ADV_ROW_ALIGN_Y);

  toggle_ui_create(&s_adv_toggle, s_screen);
  lv_obj_align(s_adv_toggle.obj, LV_ALIGN_BOTTOM_RIGHT, -18, ADV_ROW_ALIGN_Y);
  toggle_ui_set(&s_adv_toggle, host_link_ble_is_active());

  lv_obj_t *hint = lv_label_create(s_screen);
  lv_label_set_text(hint, "OK: toggle advertising   BACK: exit");
  lv_obj_set_style_text_color(hint, current_theme.text_main, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, HINT_ALIGN_Y);

  refresh_status();

  if (main_group != NULL) {
    lv_group_add_obj(main_group, s_screen);
    lv_group_focus_obj(s_screen);
  }

  if (s_nav_timer == NULL) {
    s_nav_timer = lv_timer_create(nav_timer_cb, NAV_TIMER_PERIOD_MS, NULL);
  }

  lv_screen_load(s_screen);
}

static void refresh_status(void) {
  if (s_status == NULL) {
    return;
  }
  bool adv = host_link_ble_is_active();
  bool connected = host_link_ble_is_connected();
  lv_label_set_text_fmt(s_status,
                        "Advertising: %s   App: %s",
                        adv ? "ON" : "OFF",
                        connected ? "connected" : "none");
  toggle_ui_set(&s_adv_toggle, adv);
}

static void nav_timer_cb(lv_timer_t *t) {
  if (lv_screen_active() != s_screen) {
    lv_timer_delete(t);
    s_nav_timer = NULL;
    return;
  }
  if (ui_input_is_locked()) {
    return;
  }

  bool ok = ok_button_is_down();
  bool back = back_button_is_down();

  if ((back && !s_btn_back_last) || left_button_is_down()) {
    s_btn_back_last = back;
    ui_switch_screen(SCREEN_BLE_MENU);
    return;
  }

  if (ok && !s_btn_ok_last) {
    if (host_link_ble_is_active()) {
      host_link_ble_stop();
    } else {
      host_link_ble_start();
    }
    refresh_status();
  }

  refresh_status(); // reflect async connection changes

  s_btn_ok_last = ok;
  s_btn_back_last = back;
}
