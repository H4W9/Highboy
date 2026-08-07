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

#ifndef UI_TERMINAL_H
#define UI_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/** @brief Create a styled terminal text area. */
lv_obj_t *terminal_ui_create(lv_obj_t *parent,
                             lv_coord_t width,
                             lv_coord_t height,
                             lv_align_t align,
                             lv_coord_t x_ofs,
                             lv_coord_t y_ofs);

#ifdef __cplusplus
}
#endif

#endif // UI_TERMINAL_H
