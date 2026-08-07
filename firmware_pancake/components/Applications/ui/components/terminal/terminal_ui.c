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

#include "terminal_ui.h"

#include "ui_theme.h"

lv_obj_t *terminal_ui_create(lv_obj_t *parent,
                             lv_coord_t width,
                             lv_coord_t height,
                             lv_align_t align,
                             lv_coord_t x_ofs,
                             lv_coord_t y_ofs) {
  lv_obj_t *ta = lv_textarea_create(parent);
  lv_obj_set_size(ta, width, height);
  lv_obj_align(ta, align, x_ofs, y_ofs);
  lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, 0);
  lv_obj_set_style_bg_color(ta, current_theme.bg_primary, 0);
  lv_obj_set_style_text_color(ta, current_theme.text_main, 0);
  lv_obj_set_style_radius(ta, 0, 0);
  lv_obj_set_style_border_width(ta, 1, 0);
  lv_obj_set_style_border_color(ta, current_theme.border_accent, 0);
  lv_obj_set_style_border_width(ta, 1, LV_STATE_FOCUS_KEY);
  lv_obj_set_style_border_color(ta, current_theme.border_accent, LV_STATE_FOCUS_KEY);
  lv_obj_set_style_outline_width(ta, 0, LV_STATE_FOCUS_KEY);
  return ta;
}
