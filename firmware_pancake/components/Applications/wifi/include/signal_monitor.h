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

#ifndef SIGNAL_MONITOR_H
#define SIGNAL_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Start monitoring signal strength of a specific BSSID.
 *
 * @param bssid    MAC address of the target AP (6 bytes).
 * @param channel  Wi-Fi channel to monitor.
 */
void signal_monitor_start(const uint8_t *bssid, uint8_t channel);

/**
 * @brief Stop the signal monitor.
 */
void signal_monitor_stop(void);

/**
 * @brief Get the latest RSSI reading from the monitored AP.
 *
 * @return RSSI in dBm.
 */
int8_t signal_monitor_get_rssi(void);

/**
 * @brief Get time in ms since the last signal was seen.
 *
 * @return Milliseconds since last seen, or 0 if not implemented.
 */
uint32_t signal_monitor_get_last_seen_ms(void);

#ifdef __cplusplus
}
#endif

#endif // SIGNAL_MONITOR_H
