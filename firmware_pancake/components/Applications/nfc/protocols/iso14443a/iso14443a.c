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
 * @file iso14443a.c
 * @brief ISO14443A CRC_A calculation.
 * Reference: ISO/IEC 14443-3A.
 */
#include "iso14443a.h"

#define ISO14443A_CRC_INIT 0x6363U
#define ISO14443A_CRC_LEN  2U

void iso14443a_crc(const uint8_t *data, size_t len, uint8_t crc[2]) {
  uint32_t wCrc = ISO14443A_CRC_INIT;
  for (size_t i = 0; i < len; i++) {
    uint8_t bt = data[i];
    bt = (bt ^ (uint8_t)(wCrc & 0x00FF));
    bt = (bt ^ (bt << 4));
    wCrc = (wCrc >> 8) ^ ((uint32_t)bt << 8) ^ ((uint32_t)bt << 3) ^ ((uint32_t)bt >> 4);
  }
  crc[0] = (uint8_t)(wCrc & 0xFF);
  crc[1] = (uint8_t)((wCrc >> 8) & 0xFF);
}

bool iso14443a_check_crc(const uint8_t *data, size_t len) {
  if (len < (ISO14443A_CRC_LEN + 1U))
    return false;
  uint8_t crc[2];
  iso14443a_crc(data, len - ISO14443A_CRC_LEN, crc);
  return (crc[0] == data[len - 2]) && (crc[1] == data[len - 1]);
}
