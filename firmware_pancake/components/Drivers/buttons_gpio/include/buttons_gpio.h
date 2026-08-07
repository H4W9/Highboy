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

#ifndef BUTTONS_GPIO_H
#define BUTTONS_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "pin_def.h"

typedef struct {
  uint32_t gpio;
  bool last_state;
  bool pressed_flag;
} button_t;

/**
 * @brief Virtual button zones (Pancake touch-only build).
 *
 * On Pancake there are no physical buttons; the six virtual buttons are
 * driven from FT6336 touch zones. lv_port_indev computes the active zone
 * each read cycle and feeds it here so every screen that polls
 * *_button_is_down() responds to touch unchanged.
 */
typedef enum {
  BTN_ZONE_NONE = -1,
  BTN_ZONE_UP = 0,
  BTN_ZONE_DOWN,
  BTN_ZONE_LEFT,
  BTN_ZONE_RIGHT,
  BTN_ZONE_OK,
  BTN_ZONE_BACK,
} btn_zone_t;

/**
 * @brief Set the currently-touched virtual button zone (or BTN_ZONE_NONE).
 *        Called from the touch input layer each poll cycle.
 */
void buttons_set_active_zone(btn_zone_t zone);

/**
 * @brief Initialize GPIO buttons.
 */
void buttons_init(void);

/**
 * @brief Poll button states. Call periodically from a task.
 */
void buttons_task(void);

/**
 * @brief Check if the up button was pressed since last check.
 *
 * @return true if pressed, false otherwise.
 */
bool up_button_pressed(void);

/**
 * @brief Check if the down button was pressed since last check.
 *
 * @return true if pressed, false otherwise.
 */
bool down_button_pressed(void);

/**
 * @brief Check if the left button was pressed since last check.
 *
 * @return true if pressed, false otherwise.
 */
bool left_button_pressed(void);

/**
 * @brief Check if the right button was pressed since last check.
 *
 * @return true if pressed, false otherwise.
 */
bool right_button_pressed(void);

/**
 * @brief Check if the ok button was pressed since last check.
 *
 * @return true if pressed, false otherwise.
 */
bool ok_button_pressed(void);

/**
 * @brief Check if the back button was pressed since last check.
 *
 * @return true if pressed, false otherwise.
 */
bool back_button_pressed(void);

/**
 * @brief Check if the up button is currently held down.
 *
 * @return true if held, false otherwise.
 */
bool up_button_is_down(void);

/**
 * @brief Check if the down button is currently held down.
 *
 * @return true if held, false otherwise.
 */
bool down_button_is_down(void);

/**
 * @brief Check if the left button is currently held down.
 *
 * @return true if held, false otherwise.
 */
bool left_button_is_down(void);

/**
 * @brief Check if the right button is currently held down.
 *
 * @return true if held, false otherwise.
 */
bool right_button_is_down(void);

/**
 * @brief Check if the ok button is currently held down.
 *
 * @return true if held, false otherwise.
 */
bool ok_button_is_down(void);

/**
 * @brief Check if the back button is currently held down.
 *
 * @return true if held, false otherwise.
 */
bool back_button_is_down(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTONS_GPIO_H
