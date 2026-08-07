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

#ifndef UI_PAGE_DOTS_H
#define UI_PAGE_DOTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define PAGE_DOTS_MAX 16

typedef struct {
  lv_obj_t *container;
  lv_obj_t *dots[PAGE_DOTS_MAX];
  int total;
  int current;
} page_dots_t;

/** @brief Create a page dots indicator centered on the current page. */
page_dots_t page_dots_create(lv_obj_t *parent, int total, lv_align_t align, int x_ofs, int y_ofs);

/** @brief Update which page is selected. */
void page_dots_set(page_dots_t *pd, int index);

/** @brief Show the page dots. */
void page_dots_show(page_dots_t *pd);

/** @brief Hide the page dots. */
void page_dots_hide(page_dots_t *pd);

#ifdef __cplusplus
}
#endif

#endif // UI_PAGE_DOTS_H
