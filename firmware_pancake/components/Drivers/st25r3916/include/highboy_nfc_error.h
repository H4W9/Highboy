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

#ifndef HIGHBOY_NFC_ERROR_H
#define HIGHBOY_NFC_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Base offset for High Boy NFC specific errors.
 * Fits within the user-defined range of ESP-IDF error codes.
 */
#define HIGHBOY_NFC_ERR_BASE 0x3000

/** @name Hardware & Transport Errors */
#define HIGHBOY_NFC_ERR_SPI_INIT (HIGHBOY_NFC_ERR_BASE + 0x01)
#define HIGHBOY_NFC_ERR_SPI_XFER (HIGHBOY_NFC_ERR_BASE + 0x02)
#define HIGHBOY_NFC_ERR_CHIP_ID  (HIGHBOY_NFC_ERR_BASE + 0x03)

/** @name Protocol & RF Errors */
#define HIGHBOY_NFC_ERR_CRC       (HIGHBOY_NFC_ERR_BASE + 0x10)
#define HIGHBOY_NFC_ERR_COLLISION (HIGHBOY_NFC_ERR_BASE + 0x11)
#define HIGHBOY_NFC_ERR_NO_CARD   (HIGHBOY_NFC_ERR_BASE + 0x12)
#define HIGHBOY_NFC_ERR_AUTH      (HIGHBOY_NFC_ERR_BASE + 0x13)
#define HIGHBOY_NFC_ERR_FIELD     (HIGHBOY_NFC_ERR_BASE + 0x14)

/**
 * @brief Resolve a High Boy error code to a human-readable string.
 *
 * @param err The error code (esp_err_t).
 * @return const char* String description.
 */
const char *highboy_nfc_err_to_name(esp_err_t err);

#ifdef __cplusplus
}
#endif

#endif // HIGHBOY_NFC_ERROR_H
