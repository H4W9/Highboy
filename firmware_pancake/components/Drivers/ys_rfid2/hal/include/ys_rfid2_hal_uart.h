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

#ifndef YS_RFID2_HAL_UART_H
#define YS_RFID2_HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief HAL UART configuration.
 */
typedef struct {
  int port;
  int baud_rate;
  int tx_pin;
  int rx_pin;
} ys_rfid2_hal_uart_config_t;

/**
 * @brief Initialize the UART peripheral.
 *
 * @param config  Pointer to configuration. Caller retains ownership.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if config is NULL
 *   - ESP_ERR_INVALID_STATE if already initialized
 */
esp_err_t ys_rfid2_hal_uart_init(const ys_rfid2_hal_uart_config_t *config);

/**
 * @brief Deinitialize the UART peripheral and release resources.
 */
void ys_rfid2_hal_uart_deinit(void);

/**
 * @brief Read bytes from UART.
 *
 * @param[out] out_data    Buffer to store received bytes.
 * @param      len         Maximum number of bytes to read.
 * @param      timeout_ms  Read timeout in milliseconds.
 * @return Number of bytes actually read, or -1 on error.
 */
int ys_rfid2_hal_uart_read(uint8_t *out_data, size_t len, uint32_t timeout_ms);

/**
 * @brief Write bytes to UART.
 *
 * @param data  Buffer containing bytes to send.
 * @param len   Number of bytes to send.
 * @return
 *   - ESP_OK on success
 *   - ESP_FAIL on write error
 */
esp_err_t ys_rfid2_hal_uart_write(const uint8_t *data, size_t len);

/**
 * @brief Flush the UART input buffer.
 */
void ys_rfid2_hal_uart_flush(void);

#ifdef __cplusplus
}
#endif

#endif // YS_RFID2_HAL_UART_H
