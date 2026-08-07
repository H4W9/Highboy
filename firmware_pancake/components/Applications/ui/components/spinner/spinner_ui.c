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

#include "spinner_ui.h"

#include "ui_theme.h"

#define ARC_WIDTH   3
#define ARC_SPAN    90
#define SPIN_STEP   12
#define SPIN_PERIOD 40

static void spin_cb(lv_timer_t *t) {
  lv_obj_t *arc = (lv_obj_t *)lv_timer_get_user_data(t);
  if (arc == NULL)
    return;

  int32_t start = lv_arc_get_bg_angle_start(arc);
  start = (start + SPIN_STEP) % 360;
  lv_arc_set_bg_angles(arc, start, start + ARC_SPAN);
}

spinner_ui_t spinner_ui_create(lv_obj_t *parent, int32_t size) {
  spinner_ui_t s = {0};

  s.obj = lv_arc_create(parent);
  lv_obj_set_size(s.obj, size, size);
  lv_obj_remove_flag(s.obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_style_all(s.obj);

  /* Track (full background ring, subtle) */
  lv_obj_set_style_arc_width(s.obj, ARC_WIDTH, LV_PART_MAIN);
  lv_obj_set_style_arc_color(s.obj, current_theme.border_inactive, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(s.obj, LV_OPA_40, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(s.obj, true, LV_PART_MAIN);

  /* Spinning arc (accent color) */
  lv_obj_set_style_arc_width(s.obj, ARC_WIDTH, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(s.obj, current_theme.border_accent, LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(s.obj, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(s.obj, true, LV_PART_INDICATOR);

  /* Hide knob */
  lv_obj_set_style_bg_opa(s.obj, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_pad_all(s.obj, 0, LV_PART_KNOB);

  lv_arc_set_range(s.obj, 0, 360);
  lv_arc_set_bg_angles(s.obj, 0, ARC_SPAN);
  lv_arc_set_value(s.obj, 0);

  s.timer = lv_timer_create(spin_cb, SPIN_PERIOD, s.obj);

  return s;
}

void spinner_ui_show(spinner_ui_t *s) {
  if (s == NULL)
    return;
  if (s->obj != NULL)
    lv_obj_remove_flag(s->obj, LV_OBJ_FLAG_HIDDEN);
  if (s->timer != NULL)
    lv_timer_resume(s->timer);
}

void spinner_ui_hide(spinner_ui_t *s) {
  if (s == NULL)
    return;
  if (s->obj != NULL)
    lv_obj_add_flag(s->obj, LV_OBJ_FLAG_HIDDEN);
  if (s->timer != NULL)
    lv_timer_pause(s->timer);
}

void spinner_ui_delete(spinner_ui_t *s) {
  if (s == NULL)
    return;
  if (s->timer != NULL) {
    lv_timer_delete(s->timer);
    s->timer = NULL;
  }
  if (s->obj != NULL) {
    lv_obj_del(s->obj);
    s->obj = NULL;
  }
}
