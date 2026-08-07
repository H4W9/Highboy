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

#ifndef RFID_STORAGE_H
#define RFID_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "ys_rfid2_types.h"
#include "rfid_types.h"

#define RFID_STORAGE_MAX_ENTRIES 32
#define RFID_STORAGE_NAME_MAX    32

/**
 * @brief Full RFID card entry for save/load.
 */
typedef struct {
  char name[RFID_STORAGE_NAME_MAX];
  ys_rfid2_raw_data_t raw;
  rfid_decoded_data_t decoded;
  bool is_decoded;
  int64_t timestamp_ms;
} rfid_storage_entry_t;

/**
 * @brief Lightweight info struct for listing (no raw data).
 */
typedef struct {
  char name[RFID_STORAGE_NAME_MAX];
  const char *protocol_name;
  uint32_t card_number;
  uint16_t facility_code;
} rfid_storage_info_t;

/**
 * @brief Initialize RFID storage.
 */
void rfid_storage_init(void);

/**
 * @brief Get number of stored entries.
 */
int rfid_storage_count(void);

/**
 * @brief Save a card entry to storage.
 *
 * @param name   Entry name. Must not be NULL.
 * @param raw    Raw card data.
 * @param decoded Decoded data (may be empty if is_decoded is false).
 * @param is_decoded Whether decoded data is valid.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if name or raw is NULL
 *   - ESP_ERR_NO_MEM if storage is full
 *   - ESP_FAIL on write error
 */
esp_err_t rfid_storage_save(const char *name,
                            const ys_rfid2_raw_data_t *raw,
                            const rfid_decoded_data_t *decoded,
                            bool is_decoded);

/**
 * @brief Load a full entry by index.
 *
 * @param      index  Entry index (0-based).
 * @param[out] out_entry  Loaded entry data.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if out_entry is NULL or index out of range
 *   - ESP_ERR_NOT_FOUND if entry does not exist
 */
esp_err_t rfid_storage_load(int index, rfid_storage_entry_t *out_entry);

/**
 * @brief Get lightweight info for an entry.
 *
 * @param      index  Entry index.
 * @param[out] out_info  Lightweight entry info.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NOT_FOUND if entry does not exist
 */
esp_err_t rfid_storage_get_info(int index, rfid_storage_info_t *out_info);

/**
 * @brief List all stored entry infos.
 *
 * @param[out] out_infos  Output array.
 * @param      max        Maximum entries to fill.
 * @return Number of entries listed.
 */
int rfid_storage_list(rfid_storage_info_t *out_infos, int max);

/**
 * @brief Delete an entry by index.
 *
 * @param index  Entry index.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NOT_FOUND if entry does not exist
 */
esp_err_t rfid_storage_delete(int index);

/**
 * @brief Find an entry by raw card ID string.
 *
 * @param      id_str     10-digit card ID string.
 * @param[out] out_index  Matching entry index.
 * @return
 *   - ESP_OK if found
 *   - ESP_ERR_NOT_FOUND if no match
 */
esp_err_t rfid_storage_find_by_id(const char *id_str, int *out_index);

#ifdef __cplusplus
}
#endif

#endif // RFID_STORAGE_H
