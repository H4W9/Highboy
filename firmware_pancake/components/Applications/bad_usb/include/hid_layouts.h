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

#ifndef HID_LAYOUTS_H
#define HID_LAYOUTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief Type a string using the US keyboard layout.
 *
 * Translates each character to the corresponding HID keycode + modifier
 * and sends it through the HAL.
 *
 * @param str  Null-terminated string to type.
 */
void hid_layouts_type_string_us(const char *str);

/**
 * @brief Type a string using the Brazilian ABNT2 keyboard layout.
 *
 * Handles dead-key sequences for accented characters (e.g. a, e, c)
 * and remapped punctuation keys specific to the ABNT2 layout.
 *
 * @param str  Null-terminated UTF-8 string to type.
 */
void hid_layouts_type_string_abnt2(const char *str);

#ifdef __cplusplus
}
#endif

#endif // HID_LAYOUTS_H
