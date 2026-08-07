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

#ifndef MSGBOX_UI_H
#define MSGBOX_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef void (*msgbox_cb_t)(bool confirm);

/** @brief Open a message box with optional OK and Cancel buttons. */
void msgbox_open(
    const char *icon, const char *msg, const char *btn_ok, const char *btn_cancel, msgbox_cb_t cb);

/** @brief Close the currently open message box. */
void msgbox_close(void);

/** @brief Check if a message box is currently open. */
bool msgbox_is_open(void);

#ifdef __cplusplus
}
#endif

#endif // MSGBOX_UI_H