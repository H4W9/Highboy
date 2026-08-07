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

#ifndef UI_BLE_SPAM_H
#define UI_BLE_SPAM_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Open the BLE spam screen. */
void ui_ble_spam_open(void);

/** @brief Set the displayed spam attack name. */
void ui_ble_spam_set_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif // UI_BLE_SPAM_H
