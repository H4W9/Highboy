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

#ifndef TARGET_SCANNER_H
#define TARGET_SCANNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Record for a client discovered on a target AP.
 */
typedef struct {
  uint8_t client_mac[6];
  int8_t rssi;
} target_scanner_record_t;

/**
 * @brief Start a targeted client scan on a specific AP.
 *
 * @param target_bssid  BSSID of the target AP (6 bytes).
 * @param channel       Wi-Fi channel of the target AP.
 * @return true on success, false on failure.
 */
bool target_scanner_start(const uint8_t *target_bssid, uint8_t channel);

/**
 * @brief Get results after the scan is complete (blocks until done).
 *
 * @param[out] out_count  Number of results.
 * @return Pointer to result array, or NULL if still scanning.
 */
target_scanner_record_t *target_scanner_get_results(uint16_t *out_count);

/**
 * @brief Get live results while the scan may still be running.
 *
 * @param[out] out_count     Number of results fetched so far.
 * @param[out] is_scanning   true if the scan is still in progress.
 * @return Pointer to result array, or NULL on failure.
 */
target_scanner_record_t *target_scanner_get_live_results(uint16_t *out_count, bool *is_scanning);

/**
 * @brief Free cached target scan results and notify C5.
 */
void target_scanner_free_results(void);

/**
 * @brief Save target scan results to internal flash via SPI.
 *
 * @return true on success, false on failure.
 */
bool target_scanner_save_results_to_internal_flash(void);

/**
 * @brief Save target scan results to SD card via SPI.
 *
 * @return true on success, false on failure.
 */
bool target_scanner_save_results_to_sd_card(void);

#ifdef __cplusplus
}
#endif

#endif // TARGET_SCANNER_H
