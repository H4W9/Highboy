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

#ifndef SPINNER_UI_H
#define SPINNER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
  lv_obj_t *obj;
  lv_timer_t *timer;
} spinner_ui_t;

/**
 * @brief Create a ring spinner (arc rotating).
 *
 * @param parent  Parent LVGL object.
 * @param size    Diameter in pixels.
 * @return spinner handle.
 */
spinner_ui_t spinner_ui_create(lv_obj_t *parent, int32_t size);

/** @brief Show the spinner. */
void spinner_ui_show(spinner_ui_t *s);

/** @brief Hide the spinner. */
void spinner_ui_hide(spinner_ui_t *s);

/** @brief Delete the spinner and free its resources. */
void spinner_ui_delete(spinner_ui_t *s);

#ifdef __cplusplus
}
#endif

#endif // SPINNER_UI_H
