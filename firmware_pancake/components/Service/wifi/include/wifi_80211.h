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

#ifndef WIFI_80211_H
#define WIFI_80211_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WIFI_ETHERTYPE_EAPOL 0x888E

/**
 * @brief 802.11 frame control field (IEEE Std 802.11-2020, Section 9.2.4.1).
 */
typedef struct {
  uint16_t protocol : 2;
  uint16_t type : 2;
  uint16_t subtype : 4;
  uint16_t to_ds : 1;
  uint16_t from_ds : 1;
  uint16_t more_frag : 1;
  uint16_t retry : 1;
  uint16_t pwr_mgt : 1;
  uint16_t more_data : 1;
  uint16_t protected_frame : 1;
  uint16_t order : 1;
} __attribute__((packed)) wifi_frame_control_t;

/**
 * @brief 802.11 MAC header (IEEE Std 802.11-2020, Section 9.3.3.2).
 */
typedef struct {
  uint16_t frame_control;
  uint16_t duration;
  uint8_t addr1[6]; // Receiver / Destination
  uint8_t addr2[6]; // Transmitter / Source
  uint8_t addr3[6]; // BSSID
  uint16_t seq_ctrl;
} __attribute__((packed)) wifi_mac_header_t;

/**
 * @brief LLC/SNAP header for 802.11 data frames (IEEE Std 802.2).
 */
typedef struct {
  uint8_t dsap;
  uint8_t ssap;
  uint8_t control;
  uint8_t oui[3];
  uint16_t type; // Big-endian EtherType
} __attribute__((packed)) wifi_llc_snap_t;

#ifdef __cplusplus
}
#endif

#endif // WIFI_80211_H
