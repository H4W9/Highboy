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

#ifndef SKIMMER_DETECTOR_H
#define SKIMMER_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define SKIMMER_DETECTOR_MAX_RESULTS 50

/**
 * @brief Record for a detected potential BLE skimmer device.
 */
typedef struct {
  uint8_t addr[6];
  uint8_t addr_type;
  int8_t rssi;
  char name[32];
  uint32_t last_seen;
  char detection_reason[32];
} skimmer_detector_record_t;

/**
 * @brief Start scanning for potential BLE skimmer devices.
 *
 * @return true if started, false on failure.
 */
bool skimmer_detector_start(void);

/**
 * @brief Stop the skimmer detection scan.
 */
void skimmer_detector_stop(void);

/**
 * @brief Get the list of detected potential skimmers.
 *
 * @param[out] out_count Pointer to store the number of results.
 * @return Pointer to the results array, or NULL if empty.
 */
skimmer_detector_record_t *skimmer_detector_get_results(uint16_t *out_count);

/**
 * @brief Clear all detected skimmer results.
 */
void skimmer_detector_clear_results(void);

#ifdef __cplusplus
}
#endif

#endif // SKIMMER_DETECTOR_H
