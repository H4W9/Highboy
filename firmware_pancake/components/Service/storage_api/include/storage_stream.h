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

#ifndef STORAGE_STREAM_H
#define STORAGE_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/** Opaque handle for a streaming file session. */
typedef struct storage_stream *storage_stream_t;

/**
 * @brief Open a file for streaming read/write.
 *
 * @param path  Absolute file path.
 * @param mode  fopen-style mode string ("r", "w", "a", etc.).
 * @return Stream handle, or NULL on failure.
 */
storage_stream_t storage_stream_open(const char *path, const char *mode);

/**
 * @brief Write data to an open stream.
 *
 * @param stream  Stream handle.
 * @param data    Pointer to data buffer.
 * @param size    Number of bytes to write.
 * @return ESP_OK on success, ESP_FAIL on partial write.
 */
esp_err_t storage_stream_write(storage_stream_t stream, const void *data, size_t size);

/**
 * @brief Read data from an open stream.
 *
 * @param stream              Stream handle.
 * @param buf                 Destination buffer.
 * @param size                Maximum bytes to read.
 * @param[out] out_bytes_read  Actual bytes read (can be NULL).
 * @return ESP_OK on success, ESP_FAIL if no data was read.
 */
esp_err_t
storage_stream_read(storage_stream_t stream, void *buf, size_t size, size_t *out_bytes_read);

/**
 * @brief Flush buffered data to disk.
 *
 * @param stream  Stream handle.
 * @return ESP_OK on success.
 */
esp_err_t storage_stream_flush(storage_stream_t stream);

/**
 * @brief Close the stream and free resources.
 *
 * @param stream  Stream handle. NULL is safe.
 * @return ESP_OK on success, ESP_FAIL if fclose fails.
 */
esp_err_t storage_stream_close(storage_stream_t stream);

/**
 * @brief Check if the stream is currently open.
 *
 * @param stream  Stream handle.
 * @return true if open.
 */
bool storage_stream_is_open(storage_stream_t stream);

/**
 * @brief Get the total number of bytes written to the stream.
 *
 * @param stream  Stream handle.
 * @return Bytes written, or 0 if stream is NULL.
 */
size_t storage_stream_bytes_written(storage_stream_t stream);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_STREAM_H
