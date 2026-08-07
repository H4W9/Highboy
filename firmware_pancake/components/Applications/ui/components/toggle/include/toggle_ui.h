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

#ifndef UI_TOGGLE_H
#define UI_TOGGLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "lvgl.h"

typedef struct {
  lv_obj_t *obj;
  lv_obj_t *dot;
  bool state;
} toggle_ui_t;

/** @brief Create a toggle switch on the given parent. */
void toggle_ui_create(toggle_ui_t *toggle, lv_obj_t *parent);

/** @brief Set the toggle state. */
void toggle_ui_set(toggle_ui_t *toggle, bool on);

/** @brief Toggle the current state. */
void toggle_ui_toggle(toggle_ui_t *toggle);

/** @brief Get the current toggle state. */
bool toggle_ui_get(toggle_ui_t *toggle);

#ifdef __cplusplus
}
#endif

#endif // UI_TOGGLE_H
