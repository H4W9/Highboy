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
 * @file storage_write.h
 * @brief Write operations — backend agnostic.
 */

#ifndef STORAGE_WRITE_H
#define STORAGE_WRITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t storage_write_string(const char *path, const char *data);
esp_err_t storage_append_string(const char *path, const char *data);

esp_err_t storage_write_binary(const char *path, const void *data, size_t size);
esp_err_t storage_append_binary(const char *path, const void *data, size_t size);

esp_err_t storage_write_line(const char *path, const char *line);
esp_err_t storage_append_line(const char *path, const char *line);

esp_err_t storage_write_formatted(const char *path, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
esp_err_t storage_append_formatted(const char *path, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

esp_err_t storage_write_buffer(const char *path, const void *buffer, size_t size);
esp_err_t storage_write_bytes(const char *path, const uint8_t *bytes, size_t count);
esp_err_t storage_write_byte(const char *path, uint8_t byte);
esp_err_t storage_write_int(const char *path, int32_t value);
esp_err_t storage_write_float(const char *path, float value);

esp_err_t storage_write_csv_row(const char *path, const char **columns, size_t num_columns);
esp_err_t storage_append_csv_row(const char *path, const char **columns, size_t num_columns);

#ifdef __cplusplus
}
#endif

#endif