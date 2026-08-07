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

#ifndef UI_CHART_H
#define UI_CHART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define CHART_MAX_POINTS 20

typedef void (*chart_btn_cb_t)(int btn_index);

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *chart;
  lv_chart_series_t *series;
  lv_obj_t *panel;
  lv_obj_t *btns[2];
  lv_obj_t *btn_dots[2];
  int btn_count;
  int btn_sel;
  chart_btn_cb_t btn_cb;
} chart_ui_t;

/** @brief Create a bar chart screen with header and bottom panel. */
chart_ui_t chart_ui_create(lv_obj_t *parent, const char *title);

/** @brief Add a button to the bottom panel (max 2). */
void chart_ui_add_btn(chart_ui_t *ch, const char *label, lv_color_t dot_color);

/** @brief Set the button press callback. */
void chart_ui_set_btn_cb(chart_ui_t *ch, chart_btn_cb_t cb);

/** @brief Append a data point to the chart. */
void chart_ui_add_point(chart_ui_t *ch, int32_t value);

/** @brief Set all chart points at once. */
void chart_ui_set_points(chart_ui_t *ch, const int32_t *values, int count);

/** @brief Dim or undim a button by index. */
void chart_ui_set_btn_dim(chart_ui_t *ch, int index, bool dim);

/** @brief Move button selection to the next button. */
void chart_ui_btn_next(chart_ui_t *ch);

/** @brief Move button selection to the previous button. */
void chart_ui_btn_prev(chart_ui_t *ch);

/** @brief Trigger a press on the currently selected button. */
void chart_ui_btn_press(chart_ui_t *ch);

#ifdef __cplusplus
}
#endif

#endif // UI_CHART_H
