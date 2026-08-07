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

#ifndef WIFI_DEAUTH_UI_H
#define WIFI_DEAUTH_UI_H

#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the target AP for deauth attack.
 *
 * @param ap  Pointer to the target AP record. Must not be NULL.
 */
void ui_wifi_deauth_set_target(wifi_ap_record_t *ap);

/**
 * @brief Open the Wi-Fi deauth screen.
 */
void ui_wifi_deauth_open(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_DEAUTH_UI_H
