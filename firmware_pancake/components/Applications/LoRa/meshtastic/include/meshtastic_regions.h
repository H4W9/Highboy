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

#ifndef MESHTASTIC_REGIONS_H
#define MESHTASTIC_REGIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Regulatory region identifiers.
 *
 * Values match LoRaConfig.RegionCode in the official Meshtastic
 * protobuf definition (config.proto).
 */
typedef enum {
  MT_REGION_UNSET = 0,
  MT_REGION_US = 1,
  MT_REGION_EU_433 = 2,
  MT_REGION_EU_868 = 3,
  MT_REGION_CN = 4,
  MT_REGION_JP = 5,
  MT_REGION_ANZ = 6,
  MT_REGION_KR = 7,
  MT_REGION_TW = 8,
  MT_REGION_RU = 9,
  MT_REGION_IN = 10,
  MT_REGION_NZ_865 = 11,
  MT_REGION_TH = 12,
  MT_REGION_LORA_24 = 13,
  MT_REGION_UA_433 = 14,
  MT_REGION_UA_868 = 15,
  MT_REGION_MY_433 = 16,
  MT_REGION_MY_919 = 17,
  MT_REGION_SG_923 = 18,
  MT_REGION_PH_433 = 19,
  MT_REGION_PH_868 = 20,
  MT_REGION_PH_915 = 21,
  MT_REGION_ANZ_433 = 22,
  MT_REGION_KZ_433 = 23,
  MT_REGION_KZ_863 = 24,
  MT_REGION_NP_865 = 25,
  MT_REGION_BR_902 = 26,
  MT_REGION_COUNT
} mt_region_t;

/**
 * @brief Static information about a regulatory region.
 */
typedef struct {
  mt_region_t id;
  const char *name;
  uint32_t freq_start_hz;
  uint32_t freq_stop_hz;
  uint8_t duty_pct;
  int8_t tx_pwr_max_dbm;
  bool is_wide_lora_ok;
} mt_region_info_t;

/**
 * @brief Look up the info table entry for a region id.
 *
 * @param id  Region identifier.
 * @return Pointer to static info table entry, or NULL if id is invalid.
 */
const mt_region_info_t *mt_region_info(mt_region_t id);

/**
 * @brief Total number of regions supported by the firmware.
 */
int mt_region_count(void);

/**
 * @brief Return the region currently persisted in NVS.
 *
 * Falls back to MT_REGION_US if NVS is empty.
 */
mt_region_t mt_region_current(void);

/**
 * @brief Persist the given region to NVS.
 */
void mt_region_set(mt_region_t id);

/**
 * @brief Compute the center frequency of a channel slot within a region.
 *
 * Divides the region band by the bandwidth to get slot count, then
 * picks slot (channel_num mod slot_count).
 *
 * @param id           Region identifier.
 * @param channel_num  Zero-based channel index.
 * @param bw_hz        LoRa bandwidth in Hz.
 * @return Center frequency in Hz. Falls back to 906875000 if the id is invalid.
 */
uint32_t mt_region_freq_for_channel(mt_region_t id, uint8_t channel_num, uint32_t bw_hz);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_REGIONS_H
