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

#ifndef UI_INTENSITY_BAR_H
#define UI_INTENSITY_BAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define INTENSITY_BAR_STEPS 5

typedef struct {
  lv_obj_t *obj;
  lv_obj_t *bars[INTENSITY_BAR_STEPS];
  int level;
} intensity_bar_t;

/** @brief Create an intensity bar on the given parent. */
void intensity_bar_create(intensity_bar_t *ib, lv_obj_t *parent);

/** @brief Set the intensity bar level. */
void intensity_bar_set(intensity_bar_t *ib, int level);

/** @brief Get the current intensity bar level. */
int intensity_bar_get(intensity_bar_t *ib);

/** @brief Increment the intensity bar level by one step. */
void intensity_bar_inc(intensity_bar_t *ib);

/** @brief Decrement the intensity bar level by one step. */
void intensity_bar_dec(intensity_bar_t *ib);

#ifdef __cplusplus
}
#endif

#endif // UI_INTENSITY_BAR_H
