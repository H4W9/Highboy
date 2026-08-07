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

#ifndef TRACKER_DETECTOR_H
#define TRACKER_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define TRACKER_DETECTOR_MAX_RESULTS 50

/**
 * @brief Known BLE tracker device types.
 */
typedef enum {
  TRACKER_DETECTOR_TYPE_UNKNOWN = 0,
  TRACKER_DETECTOR_TYPE_AIRTAG,
  TRACKER_DETECTOR_TYPE_SMARTTAG,
  TRACKER_DETECTOR_TYPE_TILE,
  TRACKER_DETECTOR_TYPE_CHIPOLO,
  TRACKER_DETECTOR_TYPE_COUNT
} tracker_detector_type_t;

#define TRACKER_DETECTOR_PAYLOAD_SAMPLE_SIZE 10

/**
 * @brief Record for a detected BLE tracker device.
 */
typedef struct {
  uint8_t addr[6];
  uint8_t addr_type;
  int8_t rssi;
  tracker_detector_type_t type;
  char type_str[16];
  uint32_t last_seen;
  uint8_t payload_sample[TRACKER_DETECTOR_PAYLOAD_SAMPLE_SIZE];
} tracker_detector_record_t;

/**
 * @brief Start scanning for BLE tracker devices.
 *
 * @return true if started, false on failure.
 */
bool tracker_detector_start(void);

/**
 * @brief Stop the tracker detection scan.
 */
void tracker_detector_stop(void);

/**
 * @brief Get the list of detected tracker devices.
 *
 * @param[out] out_count Pointer to store the number of results.
 * @return Pointer to the results array, or NULL if empty.
 */
tracker_detector_record_t *tracker_detector_get_results(uint16_t *out_count);

/**
 * @brief Clear all detected tracker results.
 */
void tracker_detector_clear_results(void);

#ifdef __cplusplus
}
#endif

#endif // TRACKER_DETECTOR_H
