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

#ifndef UI_TEXT_VIEWER_H
#define UI_TEXT_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *title_label;
  lv_obj_t *line_label;
  lv_obj_t *text_area;
  lv_obj_t *scroll_bar;
  int track_y_start;
  int track_h;
} text_viewer_t;

/** @brief Create a full-screen text viewer with title and scrollable content. */
text_viewer_t text_viewer_create(lv_obj_t *parent, const char *filename);

/** @brief Load text content from a file path. */
void text_viewer_load_file(text_viewer_t *tv, const char *path);

/** @brief Set text content directly. */
void text_viewer_set_text(text_viewer_t *tv, const char *text);

#ifdef __cplusplus
}
#endif

#endif // UI_TEXT_VIEWER_H
