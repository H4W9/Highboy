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

// Pancake (touch-only) implementation of the button API.
//
// There are no physical buttons on Pancake. The six virtual buttons
// (UP/DOWN/LEFT/RIGHT/OK/BACK) are driven from FT6336 touch zones:
// lv_port_indev computes the active zone each poll and calls
// buttons_set_active_zone(). The UI polls *_button_is_down() exactly as it
// did with GPIO buttons, so every screen becomes touch-navigable unchanged.

#include "buttons_gpio.h"

// Current touched zone and per-button one-shot press flags.
static volatile btn_zone_t s_active_zone = BTN_ZONE_NONE;
static bool s_pressed_flag[6] = {false};

void buttons_set_active_zone(btn_zone_t zone) {
  // On a fresh transition into a valid zone, latch a one-shot press flag
  // (kept for API parity; current screens use *_button_is_down()).
  static btn_zone_t last = BTN_ZONE_NONE;
  if (zone != last && zone >= BTN_ZONE_UP && zone <= BTN_ZONE_BACK) {
    s_pressed_flag[zone] = true;
  }
  last = zone;
  s_active_zone = zone;
}

static inline bool zone_down(btn_zone_t z) {
  return s_active_zone == z;
}

static inline bool zone_pressed(btn_zone_t z) {
  if (s_pressed_flag[z]) {
    s_pressed_flag[z] = false;
    return true;
  }
  return false;
}

bool up_button_pressed(void) { return zone_pressed(BTN_ZONE_UP); }
bool down_button_pressed(void) { return zone_pressed(BTN_ZONE_DOWN); }
bool left_button_pressed(void) { return zone_pressed(BTN_ZONE_LEFT); }
bool right_button_pressed(void) { return zone_pressed(BTN_ZONE_RIGHT); }
bool ok_button_pressed(void) { return zone_pressed(BTN_ZONE_OK); }
bool back_button_pressed(void) { return zone_pressed(BTN_ZONE_BACK); }

bool up_button_is_down(void) { return zone_down(BTN_ZONE_UP); }
bool down_button_is_down(void) { return zone_down(BTN_ZONE_DOWN); }
bool left_button_is_down(void) { return zone_down(BTN_ZONE_LEFT); }
bool right_button_is_down(void) { return zone_down(BTN_ZONE_RIGHT); }
bool ok_button_is_down(void) { return zone_down(BTN_ZONE_OK); }
bool back_button_is_down(void) { return zone_down(BTN_ZONE_BACK); }

void buttons_task(void) {
  // No-op: touch state is pushed via buttons_set_active_zone().
}

void buttons_init(void) {
  s_active_zone = BTN_ZONE_NONE;
  for (int i = 0; i < 6; i++) {
    s_pressed_flag[i] = false;
  }
}
