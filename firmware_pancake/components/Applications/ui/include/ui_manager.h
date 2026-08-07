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

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/** @brief Screen identifiers for the UI navigation system. */
typedef enum {
  SCREEN_NONE,
  SCREEN_HOME,
  SCREEN_MENU,
  SCREEN_WIFI_MENU,
  SCREEN_WIFI_ATTACK_MENU,
  SCREEN_WIFI_DEAUTH_ATTACK,
  SCREEN_WIFI_DEAUTH_UNICAST,
  SCREEN_WIFI_BEACON_SPAM_SIMPLE,
  SCREEN_WIFI_PROBE_FLOOD,
  SCREEN_WIFI_PROBE_FLOOD_BROADCAST,
  SCREEN_WIFI_PROBE_FLOOD_UNICAST,
  SCREEN_WIFI_AUTH_FLOOD,
  SCREEN_WIFI_PACKETS_MENU,
  SCREEN_WIFI_SNIFFER_RAW,
  SCREEN_WIFI_SNIFFER_ATTACK,
  SCREEN_WIFI_SNIFFER_HANDSHAKE,
  SCREEN_WIFI_SCAN_MENU,
  SCREEN_WIFI_SCAN_AP,
  SCREEN_WIFI_SCAN_STATIONS,
  SCREEN_WIFI_SCAN_TARGET,
  SCREEN_WIFI_SCAN_PROBE,
  SCREEN_WIFI_SCAN_MONITOR,
  SCREEN_WIFI_SCAN,
  SCREEN_WIFI_AP_LIST,
  SCREEN_WIFI_DEAUTH,
  SCREEN_WIFI_EVIL_TWIN,
  SCREEN_WIFI_BEACON_SPAM,
  SCREEN_WIFI_PROBE,
  SCREEN_BLE_MENU,
  SCREEN_BLE_SPAM_SELECT,
  SCREEN_BLE_SPAM,
  SCREEN_BADUSB_MENU,
  SCREEN_BADUSB_BROWSER,
  SCREEN_BADUSB_LAYOUT,
  SCREEN_BADUSB_CONNECT,
  SCREEN_BADUSB_RUNNING,
  SCREEN_SUBGHZ_SPECTRUM,
  SCREEN_SETTINGS,
  SCREEN_DISPLAY_SETTINGS,
  SCREEN_INTERFACE_SETTINGS,
  SCREEN_SOUND_SETTINGS,
  SCREEN_BATTERY_SETTINGS,
  SCREEN_CONNECTION_SETTINGS,
  SCREEN_CONNECT_WIFI,
  SCREEN_CONNECT_BLUETOOTH,
  SCREEN_ABOUT_SETTINGS,
  SCREEN_NFC_MENU,
  SCREEN_FILES,
  SCREEN_THEME_SELECTOR,
  SCREEN_IR_MENU,
  SCREEN_IR_RECEIVE,
  SCREEN_IR_SEND,
  SCREEN_IR_CONTROLLER,
  SCREEN_IR_SAVED,
  SCREEN_IR_BURST,
  SCREEN_COUNT
} screen_id_t;

/** @brief Initialize the UI manager and start the UI task. */
void ui_init(void);

/** @brief Perform an emergency restart of the UI task. */
void ui_hard_restart(void);

/** @brief Acquire the UI mutex for thread-safe LVGL access. */
bool ui_acquire(void);

/** @brief Release the UI mutex. */
void ui_release(void);

/** @brief Switch to a new screen by identifier. */
void ui_switch_screen(screen_id_t new_screen);

/** @brief Check if user input is temporarily locked. */
bool ui_input_is_locked(void);

#ifdef __cplusplus
}
#endif

#endif // UI_MANAGER_H
