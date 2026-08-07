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

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
  lv_obj_t *obj;
  lv_obj_t *dot;
  lv_obj_t *icon;
  lv_obj_t *label;
} button_ui_t;

/**
 * @brief Create a button with optional dot color and icon.
 *
 * @param parent     Parent LVGL object.
 * @param width      Button width.
 * @param height     Button height.
 * @param text       Button label text.
 * @param icon_path  Path to icon asset, or NULL.
 * @param dot_color  Pointer to dot color, or NULL to omit dot.
 * @return Initialized button handle.
 */
button_ui_t button_ui_create(lv_obj_t *parent,
                             lv_coord_t width,
                             lv_coord_t height,
                             const char *text,
                             const char *icon_path,
                             const lv_color_t *dot_color);

/** @brief Set the selected visual state of a button. */
void button_ui_set_selected(button_ui_t *btn, bool selected);

/** @brief Set the dimmed visual state of a button. */
void button_ui_set_dim(button_ui_t *btn, bool dim);

/** @brief Update the button label text. */
void button_ui_set_text(button_ui_t *btn, const char *text);

#ifdef __cplusplus
}
#endif

#endif // UI_BUTTON_H
