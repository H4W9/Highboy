// Copyright (c) 2026 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef RNODE_SERIAL_H
#define RNODE_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

/**
 * @brief Initialize UART0 in raw mode for the KISS protocol.
 *
 * ESP32-S3 default pins: TX=GPIO43, RX=GPIO44. 115200 8N1 no flow control.
 *
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t rnode_serial_init(void);

/**
 * @brief Write raw bytes to the UART. Blocking.
 *
 * @param data  Buffer to write.
 * @param len   Number of bytes.
 * @return Bytes written, or -1 on error.
 */
int rnode_serial_write(const uint8_t *data, size_t len);

/**
 * @brief Read raw bytes from the UART (non-blocking, timeout 0).
 *
 * @param out  Destination buffer.
 * @param cap  Destination capacity.
 * @return Bytes read (>= 0), 0 if nothing available.
 */
int rnode_serial_read(uint8_t *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif // RNODE_SERIAL_H
