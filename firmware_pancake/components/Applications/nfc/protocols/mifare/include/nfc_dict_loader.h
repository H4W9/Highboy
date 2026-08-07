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
/**
 * @file nfc_dict_loader.h
 * @brief Parser for .dic key dictionary files (hex per line, '#' comments).
 *
 * The .dic format is plain ASCII text. Each non-empty, non-comment line is
 * a hex string representing a single key. Whitespace is ignored. Lines that
 * do not match the requested key size are skipped (with a debug log).
 */
#ifndef NFC_DICT_LOADER_H
#define NFC_DICT_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief Parameters for nfc_dict_load_file().
 */
typedef struct {
  const char *sd_path;    /**< @brief SD path. May be NULL to skip SD lookup. */
  const char *flash_path; /**< @brief Flash partition fallback path. May be NULL. */
  size_t key_size;        /**< @brief Bytes per key (e.g. 6 or 16). */
  size_t max_keys;        /**< @brief Maximum keys to read. */
} nfc_dict_load_params_t;

/**
 * @brief Parameters for nfc_dict_load_dir().
 */
typedef struct {
  const char *dir;    /**< @brief Directory to scan. */
  const char *prefix; /**< @brief Filename prefix filter, NULL for any. */
  size_t key_size;    /**< @brief Bytes per key. */
  size_t max_keys;    /**< @brief Maximum keys to read across all files. */
} nfc_dict_scan_params_t;

/**
 * @brief Load a .dic file into a contiguous key buffer.
 *
 * Tries @c params->sd_path first; if missing/empty, falls back to
 * @c params->flash_path.
 *
 * @param params         Source paths and sizing.
 * @param[out] out_keys  Buffer of capacity (key_size * max_keys) bytes.
 * @param[out] out_count Number of keys actually loaded.
 * @return
 *   - ESP_OK if at least one key was read
 *   - ESP_ERR_NOT_FOUND if both paths are missing/empty
 *   - ESP_ERR_INVALID_ARG on bad arguments
 */
esp_err_t
nfc_dict_load_file(const nfc_dict_load_params_t *params, uint8_t *out_keys, size_t *out_count);

/**
 * @brief Scan a directory and load every .dic file matching the prefix.
 *
 * Files are read in directory-iteration order. Duplicates across files are
 * skipped. Returns ESP_OK even if no keys were found, as long as the
 * directory could be opened.
 *
 * @param params         Scan parameters (dir, prefix, key_size, max_keys).
 * @param[out] out_keys  Buffer of capacity (key_size * max_keys) bytes.
 * @param[out] out_count Number of unique keys loaded across all files.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NOT_FOUND if the directory cannot be opened
 *   - ESP_ERR_INVALID_ARG on bad arguments
 */
esp_err_t
nfc_dict_load_dir(const nfc_dict_scan_params_t *params, uint8_t *out_keys, size_t *out_count);

/**
 * @brief Check if @p key already exists in the contiguous buffer @p keys.
 *
 * @param keys     Buffer of (count * key_size) bytes.
 * @param count    Number of keys in the buffer.
 * @param key_size Bytes per key.
 * @param key      Candidate key.
 * @return true if a byte-for-byte match exists.
 */
bool nfc_dict_contains(const uint8_t *keys, size_t count, size_t key_size, const uint8_t *key);

/**
 * @brief Append a key to a .dic file on the SD card (creating it if needed).
 *
 * Writes one uppercase hex line followed by '\n'. Does not check for
 * duplicates — caller is expected to use @ref nfc_dict_contains first.
 *
 * @param sd_path  Path on the SD card.
 * @param key      Key bytes.
 * @param key_size Bytes per key.
 * @return ESP_OK on success, ESP_FAIL on I/O error.
 */
esp_err_t nfc_dict_append_key(const char *sd_path, const uint8_t *key, size_t key_size);

#ifdef __cplusplus
}
#endif

#endif // NFC_DICT_LOADER_H
