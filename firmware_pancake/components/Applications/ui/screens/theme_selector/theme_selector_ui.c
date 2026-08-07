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

#include "theme_selector_ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "esp_log.h"
#include "cJSON.h"

#include "ui_theme.h"
#include "ui_manager.h"
#include "tos_config.h"
#include "tos_storage_paths.h"
#include "tos_flash_paths.h"
#include "buttons_gpio.h"
#include "assets_manager.h"
#include "storage_impl.h"
#include "st7789.h"

static const char *TAG = "THEME_SELECTOR_UI";

#define TITLE_W              170
#define TITLE_H              30
#define TITLE_RADIUS         12
#define TITLE_BORDER_W       2
#define ITEM_W               210
#define ITEM_H               47
#define ITEM_RADIUS          10
#define ITEM_BORDER_NORMAL   1
#define ITEM_BORDER_SELECTED 3
#define ITEM_PAD_H           6
#define ITEM_PAD_COL         4
#define OUTER_BORDER         4
#define TOP_BORDER_H         (TITLE_H + 16)
#define TOP_AREA_BORDER_W    3

#define SWATCH_SIZE     12
#define SWATCH_RADIUS   2
#define SWATCH_BORDER_W 1
#define SWATCH_COUNT    5

#define DOT_SIZE     8
#define DOT_OFFSET_X (-12)
#define PTR_OFFSET_X (-6)

#define ITEMS_Y_OFFSET      4
#define ITEMS_CONT_PAD      2
#define ITEMS_CONT_PAD_ROW  6
#define ITEMS_CONT_X_OFFSET 4

#define SCROLL_TRACK_W        3
#define SCROLL_TRACK_DASH_W   4
#define SCROLL_TRACK_DASH_GAP 4
#define SCROLL_TRACK_X_MARGIN 9
#define SCROLL_TRACK_Y_MARGIN 10
#define SCROLL_BAR_X_OFFSET   (-4)
#define SCROLL_BAR_THUMB_H    20
#define SCROLL_BAR_ANIM_MS    200

#define THEME_NAME_MAX_LEN  32
#define THEME_COLOR_COUNT   5
#define MAX_THEMES          24
#define BUILTIN_THEME_COUNT 12

#define THEME_CONF_PATH_FMT  TOS_PATH_THEMES "/%.30s/theme.conf"
#define THEME_CONF_PATH_SIZE 96

#define NAV_TIMER_PERIOD_MS 50

typedef struct {
  char name[THEME_NAME_MAX_LEN];
  uint32_t
      colors[THEME_COLOR_COUNT]; // bg_primary, bg_secondary, border_accent, text_main, screen_base
} theme_selector_entry_t;

static const char *BUILTIN_THEMES[BUILTIN_THEME_COUNT] = {"default",
                                                          "matrix",
                                                          "cyber_blue",
                                                          "blood",
                                                          "toxic",
                                                          "ghost",
                                                          "neon_pink",
                                                          "amber",
                                                          "terminal",
                                                          "ice",
                                                          "deep_purple",
                                                          "midnight"};

static const char *COLOR_KEYS[THEME_COLOR_COUNT] = {
    "bg_primary", "bg_secondary", "border_accent", "text_main", "screen_base"};

static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_items_cont = NULL;
static lv_obj_t *s_items[MAX_THEMES];
static lv_obj_t *s_sel_dots[MAX_THEMES];
static lv_obj_t *s_active_icons[MAX_THEMES];
static lv_obj_t *s_scroll_bar = NULL;
static lv_timer_t *s_nav_timer = NULL;

static theme_selector_entry_t s_themes[MAX_THEMES];
static int s_theme_count = 0;
static int s_selected = 0;
static int s_track_y_start = 0;
static int s_track_h = 0;
static bool s_rebuilding = false;

static bool s_btn_up_last = false;
static bool s_btn_down_last = false;
static bool s_btn_ok_last = false;
static bool s_btn_back_last = false;

static uint32_t hex_str_to_u32(const char *s);
static char *read_file_alloc(const char *path);
static bool parse_conf_colors(const char *data, uint32_t out[THEME_COLOR_COUNT]);
static bool parse_json_colors(const char *data, const char *name, uint32_t out[THEME_COLOR_COUNT]);
static void scan_themes(void);
static void update_scroll_bar(void);
static void update_selection(void);
static void create_theme_item(lv_obj_t *parent, int idx);
static void apply_theme(int idx);
static void nav_timer_cb(lv_timer_t *t);

static uint32_t hex_str_to_u32(const char *s) {
  if (s == NULL)
    return 0;
  return (uint32_t)strtol(s, NULL, 16);
}

static char *read_file_alloc(const char *path) {
  FILE *f = fopen(path, "r");
  if (f == NULL)
    return NULL;

  fseek(f, 0, SEEK_END);
  int32_t sz = (int32_t)ftell(f);
  fseek(f, 0, SEEK_SET);

  if (sz <= 0) {
    fclose(f);
    return NULL;
  }

  char *buf = malloc((size_t)sz + 1);
  if (buf == NULL) {
    ESP_LOGE(TAG, "Failed to allocate read buffer for %s", path);
    fclose(f);
    return NULL;
  }

  size_t read = fread(buf, 1, (size_t)sz, f);
  fclose(f);

  if ((int32_t)read != sz) {
    ESP_LOGE(TAG, "Short read on %s: expected %ld, got %zu", path, (long)sz, read);
    free(buf);
    return NULL;
  }

  buf[sz] = '\0';
  return buf;
}

static bool parse_conf_colors(const char *data, uint32_t out[THEME_COLOR_COUNT]) {
  int found = 0;
  for (int k = 0; k < THEME_COLOR_COUNT; k++) {
    const char *p = strstr(data, COLOR_KEYS[k]);
    if (p == NULL)
      continue;
    const char *eq = strchr(p, '=');
    if (eq == NULL)
      continue;
    eq++;
    while (*eq == ' ')
      eq++;
    out[k] = hex_str_to_u32(eq);
    found++;
  }
  return found >= 3;
}

static bool parse_json_colors(const char *data, const char *name, uint32_t out[THEME_COLOR_COUNT]) {
  cJSON *root = cJSON_Parse(data);
  if (root == NULL)
    return false;

  cJSON *theme = cJSON_GetObjectItem(root, name);
  if (theme == NULL) {
    cJSON_Delete(root);
    return false;
  }

  for (int k = 0; k < THEME_COLOR_COUNT; k++) {
    cJSON *v = cJSON_GetObjectItem(theme, COLOR_KEYS[k]);
    out[k] = (cJSON_IsString(v) && v->valuestring != NULL) ? hex_str_to_u32(v->valuestring) : 0;
  }

  cJSON_Delete(root);
  return true;
}

static void scan_themes(void) {
  s_theme_count = 0;

  DIR *d = opendir(TOS_PATH_THEMES);
  if (d != NULL) {
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && s_theme_count < MAX_THEMES) {
      if (ent->d_type != DT_DIR || ent->d_name[0] == '.')
        continue;
      if (strlen(ent->d_name) > THEME_NAME_MAX_LEN - 2)
        continue;

      char path[THEME_CONF_PATH_SIZE];
      snprintf(path, sizeof(path), THEME_CONF_PATH_FMT, ent->d_name);

      char *data = read_file_alloc(path);
      if (data == NULL)
        continue;

      theme_selector_entry_t *t = &s_themes[s_theme_count];
      strncpy(t->name, ent->d_name, sizeof(t->name) - 1);
      t->name[sizeof(t->name) - 1] = '\0';

      if (parse_conf_colors(data, t->colors))
        s_theme_count++;

      free(data);
    }
    closedir(d);
  }

  if (s_theme_count == 0) {
    char *data = read_file_alloc(FLASH_CONFIG_THEMES);
    if (data != NULL) {
      for (int i = 0; i < BUILTIN_THEME_COUNT && s_theme_count < MAX_THEMES; i++) {
        theme_selector_entry_t *t = &s_themes[s_theme_count];
        strncpy(t->name, BUILTIN_THEMES[i], sizeof(t->name) - 1);
        t->name[sizeof(t->name) - 1] = '\0';
        if (parse_json_colors(data, BUILTIN_THEMES[i], t->colors))
          s_theme_count++;
      }
      free(data);
    }
  }

  s_selected = 0;
  for (int i = 0; i < s_theme_count; i++) {
    if (strcmp(s_themes[i].name, g_config_screen.theme) == 0) {
      s_selected = i;
      break;
    }
  }
}

static void update_scroll_bar(void) {
  if (s_scroll_bar == NULL || s_theme_count <= 1)
    return;

  int32_t pos =
      s_track_y_start + (s_selected * (s_track_h - SCROLL_BAR_THUMB_H)) / (s_theme_count - 1);

  if (s_rebuilding) {
    lv_obj_set_y(s_scroll_bar, pos);
    return;
  }

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, s_scroll_bar);
  lv_anim_set_values(&a, lv_obj_get_y(s_scroll_bar), pos);
  lv_anim_set_duration(&a, SCROLL_BAR_ANIM_MS);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_start(&a);
}

static void update_selection(void) {
  for (int i = 0; i < s_theme_count; i++) {
    bool is_active = (strcmp(s_themes[i].name, g_config_screen.theme) == 0);

    if (i == s_selected) {
      lv_obj_set_style_border_color(s_items[i], current_theme.border_accent, 0);
      lv_obj_set_style_border_width(s_items[i], ITEM_BORDER_SELECTED, 0);
      if (s_sel_dots[i] != NULL)
        lv_obj_remove_flag(s_sel_dots[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_set_style_border_color(s_items[i], current_theme.border_interface, 0);
      lv_obj_set_style_border_width(s_items[i], ITEM_BORDER_NORMAL, 0);
      if (s_sel_dots[i] != NULL)
        lv_obj_add_flag(s_sel_dots[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (s_active_icons[i] != NULL) {
      if (is_active)
        lv_obj_remove_flag(s_active_icons[i], LV_OBJ_FLAG_HIDDEN);
      else
        lv_obj_add_flag(s_active_icons[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  if (s_items[s_selected] != NULL)
    lv_obj_scroll_to_view(s_items[s_selected], s_rebuilding ? LV_ANIM_OFF : LV_ANIM_ON);

  update_scroll_bar();
}

static void create_theme_item(lv_obj_t *parent, int idx) {
  theme_selector_entry_t *t = &s_themes[idx];

  lv_obj_t *item = lv_obj_create(parent);
  lv_obj_set_size(item, ITEM_W, ITEM_H);
  lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(item, ITEM_RADIUS, 0);
  lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(item, current_theme.bg_primary, 0);
  lv_obj_set_style_bg_grad_color(item, current_theme.bg_secondary, 0);
  lv_obj_set_style_bg_grad_dir(item, LV_GRAD_DIR_HOR, 0);
  lv_obj_set_style_border_width(item, ITEM_BORDER_NORMAL, 0);
  lv_obj_set_style_border_color(item, current_theme.border_interface, 0);
  lv_obj_set_style_pad_left(item, ITEM_PAD_H, 0);
  lv_obj_set_style_pad_right(item, ITEM_PAD_H, 0);
  lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(item, ITEM_PAD_COL, 0);

  for (int c = 0; c < SWATCH_COUNT; c++) {
    lv_obj_t *sw = lv_obj_create(item);
    lv_obj_set_size(sw, SWATCH_SIZE, SWATCH_SIZE);
    lv_obj_set_style_radius(sw, SWATCH_RADIUS, 0);
    lv_obj_set_style_bg_color(sw, lv_color_hex(t->colors[c]), 0);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sw, lv_color_hex(t->colors[2]), 0);
    lv_obj_set_style_border_width(sw, SWATCH_BORDER_W, 0);
    lv_obj_remove_flag(sw, LV_OBJ_FLAG_SCROLLABLE);
  }

  char upper[THEME_NAME_MAX_LEN];
  strncpy(upper, t->name, sizeof(upper) - 1);
  upper[sizeof(upper) - 1] = '\0';
  for (int c = 0; upper[c]; c++) {
    if (upper[c] >= 'a' && upper[c] <= 'z')
      upper[c] -= 32;
  }

  lv_obj_t *lbl = lv_label_create(item);
  lv_label_set_text(lbl, upper);
  lv_obj_set_style_text_color(lbl, current_theme.text_main, 0);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
  lv_obj_set_flex_grow(lbl, 1);

  lv_obj_t *dot = lv_obj_create(item);
  lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dot, current_theme.text_main, 0);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(dot, 0, 0);
  lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dot, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(dot, LV_ALIGN_RIGHT_MID, DOT_OFFSET_X, 0);
  s_active_icons[idx] = dot;

  lv_image_dsc_t *pointer_dsc = assets_get("/assets/icons/pointer.bin");
  lv_obj_t *ptr = lv_image_create(item);
  if (pointer_dsc != NULL)
    lv_image_set_src(ptr, pointer_dsc);
  lv_obj_add_flag(ptr, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_FLOATING);
  lv_obj_align(ptr, LV_ALIGN_RIGHT_MID, PTR_OFFSET_X, 0);
  s_sel_dots[idx] = ptr;

  s_items[idx] = item;

  if (idx == s_selected) {
    lv_obj_set_style_border_width(item, ITEM_BORDER_SELECTED, 0);
    lv_obj_set_style_border_color(item, current_theme.border_accent, 0);
    lv_obj_remove_flag(ptr, LV_OBJ_FLAG_HIDDEN);
  }
}

static void apply_theme(int idx) {
  if (idx < 0 || idx >= s_theme_count)
    return;

  ESP_LOGI(TAG, "Applying theme: %s", s_themes[idx].name);

  ui_theme_load_from_name(s_themes[idx].name);
  strncpy(g_config_screen.theme, s_themes[idx].name, sizeof(g_config_screen.theme) - 1);
  g_config_screen.theme[sizeof(g_config_screen.theme) - 1] = '\0';
  tos_config_save(TOS_PATH_CONFIG_SCREEN, "screen");

  s_rebuilding = true;
  ui_theme_selector_open();
  s_rebuilding = false;
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
  bool ok = ok_button_is_down();
  bool back = back_button_is_down();

  if (up && !s_btn_up_last && s_theme_count > 0) {
    s_selected = (s_selected == 0) ? s_theme_count - 1 : s_selected - 1;
    update_selection();
  }
  if (down && !s_btn_down_last && s_theme_count > 0) {
    s_selected = (s_selected + 1) % s_theme_count;
    update_selection();
  }
  if (ok && !s_btn_ok_last) {
    apply_theme(s_selected);
  }
  if (back && !s_btn_back_last) {
    s_btn_back_last = back;
    ui_switch_screen(SCREEN_INTERFACE_SETTINGS);
    return;
  }

  s_btn_up_last = up;
  s_btn_down_last = down;
  s_btn_ok_last = ok;
  s_btn_back_last = back;
}

void ui_theme_selector_open(void) {
  if (s_screen != NULL) {
    lv_obj_del(s_screen);
    s_screen = NULL;
  }

  memset(s_items, 0, sizeof(s_items));
  memset(s_sel_dots, 0, sizeof(s_sel_dots));
  memset(s_active_icons, 0, sizeof(s_active_icons));
  s_scroll_bar = NULL;

  scan_themes();

  s_screen = lv_obj_create(NULL);
  lv_obj_set_size(s_screen, LCD_H_RES, LCD_V_RES);
  lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(s_screen, current_theme.screen_base, 0);
  lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(s_screen, 0, 0);
  lv_obj_set_style_border_width(s_screen, OUTER_BORDER, 0);
  lv_obj_set_style_border_color(s_screen, current_theme.border_interface, 0);
  lv_obj_set_style_radius(s_screen, 0, 0);

  lv_obj_t *top_area = lv_obj_create(s_screen);
  lv_obj_set_size(top_area, LCD_H_RES - OUTER_BORDER * 2, TOP_BORDER_H);
  lv_obj_align(top_area, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_remove_flag(top_area, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(top_area, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(top_area, TOP_AREA_BORDER_W, 0);
  lv_obj_set_style_border_color(top_area, current_theme.border_interface, 0);
  lv_obj_set_style_border_side(top_area, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_radius(top_area, 0, 0);
  lv_obj_set_style_pad_all(top_area, 0, 0);

  lv_obj_t *title_bar = lv_obj_create(top_area);
  lv_obj_set_size(title_bar, TITLE_W, TITLE_H);
  lv_obj_align(title_bar, LV_ALIGN_CENTER, 0, 0);
  lv_obj_remove_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(title_bar, TITLE_RADIUS, 0);
  lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(title_bar, current_theme.bg_primary, 0);
  lv_obj_set_style_bg_grad_color(title_bar, current_theme.bg_secondary, 0);
  lv_obj_set_style_bg_grad_dir(title_bar, LV_GRAD_DIR_HOR, 0);
  lv_obj_set_style_border_width(title_bar, TITLE_BORDER_W, 0);
  lv_obj_set_style_border_color(title_bar, current_theme.border_accent, 0);
  lv_obj_set_style_pad_all(title_bar, 0, 0);

  lv_obj_t *title_lbl = lv_label_create(title_bar);
  lv_label_set_text(title_lbl, "THEMES");
  lv_obj_set_style_text_color(title_lbl, current_theme.text_main, 0);
  lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
  lv_obj_center(title_lbl);

  int items_y = TOP_BORDER_H + ITEMS_Y_OFFSET;
  int items_h = LCD_V_RES - items_y - OUTER_BORDER - ITEMS_Y_OFFSET;

  s_items_cont = lv_obj_create(s_screen);
  lv_obj_set_size(s_items_cont, ITEM_W + 8, items_h);
  lv_obj_align(s_items_cont, LV_ALIGN_TOP_LEFT, ITEMS_CONT_X_OFFSET, items_y);
  lv_obj_set_style_bg_opa(s_items_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(s_items_cont, 0, 0);
  lv_obj_set_style_pad_all(s_items_cont, ITEMS_CONT_PAD, 0);
  lv_obj_set_style_pad_row(s_items_cont, ITEMS_CONT_PAD_ROW, 0);
  lv_obj_set_flex_flow(s_items_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(s_items_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_snap_y(s_items_cont, LV_SCROLL_SNAP_START);

  int track_x = LCD_H_RES - OUTER_BORDER - SCROLL_TRACK_X_MARGIN;
  s_track_y_start = items_y + SCROLL_TRACK_Y_MARGIN;
  s_track_h = items_h - SCROLL_TRACK_Y_MARGIN * 2;

  // Points must outlive this function (used by lv_line)
  static lv_point_precise_t track_pts[2];
  track_pts[0].x = 0;
  track_pts[0].y = 0;
  track_pts[1].x = 0;
  track_pts[1].y = s_track_h;

  lv_obj_t *track = lv_line_create(s_screen);
  lv_line_set_points(track, track_pts, 2);
  lv_obj_set_pos(track, track_x, s_track_y_start);
  lv_obj_set_style_line_color(track, current_theme.border_inactive, 0);
  lv_obj_set_style_line_opa(track, LV_OPA_COVER, 0);
  lv_obj_set_style_line_width(track, SCROLL_TRACK_W, 0);
  lv_obj_set_style_line_dash_width(track, SCROLL_TRACK_DASH_W, 0);
  lv_obj_set_style_line_dash_gap(track, SCROLL_TRACK_DASH_GAP, 0);

  lv_image_dsc_t *slide_dsc = assets_get("/assets/icons/slide_bar_v.bin");
  s_scroll_bar = lv_image_create(s_screen);
  if (slide_dsc != NULL)
    lv_image_set_src(s_scroll_bar, slide_dsc);
  lv_obj_set_pos(s_scroll_bar, track_x + SCROLL_BAR_X_OFFSET, s_track_y_start);
  lv_obj_move_foreground(s_scroll_bar);

  for (int i = 0; i < s_theme_count; i++)
    create_theme_item(s_items_cont, i);

  update_selection();

  if (s_nav_timer == NULL)
    s_nav_timer = lv_timer_create(nav_timer_cb, NAV_TIMER_PERIOD_MS, NULL);

  lv_screen_load(s_screen);
}