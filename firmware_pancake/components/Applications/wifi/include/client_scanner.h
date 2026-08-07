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

#ifndef CLIENT_SCANNER_H
#define CLIENT_SCANNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Record for a discovered client station.
 */
typedef struct {
  uint8_t bssid[6];
  uint8_t client_mac[6];
  int8_t rssi;
  uint8_t channel;
} client_scanner_record_t;

/**
 * @brief Start a client scan over SPI.
 *
 * @return true on success, false on failure.
 */
bool client_scanner_start(void);

/**
 * @brief Get cached client scan results.
 *
 * @param[out] out_count  Number of results.
 * @return Pointer to result array, or NULL if scan not ready.
 */
client_scanner_record_t *client_scanner_get_results(uint16_t *out_count);

/**
 * @brief Free cached client scan results.
 */
void client_scanner_free_results(void);

/**
 * @brief Save client scan results to internal flash via SPI.
 *
 * @return true on success, false on failure.
 */
bool client_scanner_save_results_to_internal_flash(void);

/**
 * @brief Save client scan results to SD card via SPI.
 *
 * @return true on success, false on failure.
 */
bool client_scanner_save_results_to_sd_card(void);

#ifdef __cplusplus
}
#endif

#endif // CLIENT_SCANNER_H
