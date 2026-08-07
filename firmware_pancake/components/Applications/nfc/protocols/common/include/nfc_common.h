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
 * @file nfc_common.h
 * @brief Common utilities for the NFC stack.
 */
#ifndef NFC_COMMON_H
#define NFC_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "highboy_nfc_error.h"
#include "highboy_nfc_types.h"
#include "highboy_nfc_compat.h"

/**
 * @brief Log a byte array as a hex string.
 *
 * @param label  Descriptive label printed before the hex data.
 * @param data   Pointer to the byte array.
 * @param len    Number of bytes to log.
 */
void nfc_log_hex(const char *label, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_COMMON_H */
