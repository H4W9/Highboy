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
 * @file storage_read.h
 * @brief Read operations — backend agnostic.
 */

#ifndef STORAGE_READ_H
#define STORAGE_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t storage_read_string(const char *path, char *buffer, size_t buffer_size);
esp_err_t storage_read_binary(const char *path, void *buffer, size_t size, size_t *bytes_read);

typedef void (*storage_line_callback_t)(const char *line, void *user_data);
esp_err_t
storage_read_line(const char *path, char *buffer, size_t buffer_size, uint32_t line_number);
esp_err_t storage_read_first_line(const char *path, char *buffer, size_t buffer_size);
esp_err_t storage_read_last_line(const char *path, char *buffer, size_t buffer_size);
esp_err_t storage_read_lines(const char *path, storage_line_callback_t callback, void *user_data);
esp_err_t storage_count_lines(const char *path, uint32_t *line_count);
esp_err_t
storage_read_chunk(const char *path, size_t offset, void *buffer, size_t size, size_t *bytes_read);
esp_err_t storage_read_int(const char *path, int32_t *value);
esp_err_t storage_read_float(const char *path, float *value);
esp_err_t storage_read_bytes(const char *path, uint8_t *bytes, size_t max_count, size_t *count);
esp_err_t storage_read_byte(const char *path, uint8_t *byte);

esp_err_t storage_file_contains(const char *path, const char *search, bool *found);
esp_err_t storage_count_occurrences(const char *path, const char *search, uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif