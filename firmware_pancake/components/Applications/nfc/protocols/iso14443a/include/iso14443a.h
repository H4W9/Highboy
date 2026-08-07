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
 * @file iso14443a.h
 * @brief ISO14443A types and CRC_A.
 */
#ifndef ISO14443A_H
#define ISO14443A_H

#include <stdint.h>
#include <stddef.h>
#include "highboy_nfc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_LOG_HEX_BUF_SIZE 128

/**
 * @brief Calculate CRC_A with initial value 0x6363.
 *
 * @param data  Input data buffer.
 * @param len   Length of data in bytes.
 * @param[out] crc  Output buffer for 2-byte CRC result.
 */
void iso14443a_crc(const uint8_t *data, size_t len, uint8_t crc[2]);

/**
 * @brief Verify CRC_A on received data.
 *
 * @param data  Input data with CRC in the last 2 bytes.
 * @param len   Total length including CRC bytes.
 * @return
 *   - true if CRC is valid
 *   - false otherwise
 */
bool iso14443a_check_crc(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ISO14443A_H */
