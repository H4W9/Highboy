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
 * @file spi_shim.h
 * @brief In-process replacement for the P4<->C5 SPI bridge (Pancake).
 *
 * Pancake is a single ESP32-C5: the radio work that ran on the C5
 * coprocessor now runs in the same binary. spi_bridge_send_command() calls
 * spi_shim_dispatch() first; supported command IDs are serviced directly via
 * esp_wifi (and later BLE). This keeps the entire P4 app/UI layer unchanged.
 */

#ifndef SPI_SHIM_H
#define SPI_SHIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "spi_protocol.h"

/**
 * @brief Service a bridge command in-process.
 *
 * @return ESP_OK / an error code if the command was handled; ESP_ERR_NOT_FOUND
 *         if the caller should fall through to the (legacy) SPI transport.
 */
esp_err_t spi_shim_dispatch(spi_id_t id,
                            const uint8_t *payload,
                            uint8_t len,
                            spi_header_t *out_header,
                            uint8_t *out_payload);

#ifdef __cplusplus
}
#endif

#endif // SPI_SHIM_H
