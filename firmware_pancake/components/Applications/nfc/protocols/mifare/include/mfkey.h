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
 * @file mfkey.h
 * @brief MFKey key recovery attacks for MIFARE Classic.
 */
#ifndef MFKEY_H
#define MFKEY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Recover key from two nonces (nested attack).
 *
 * @param uid  Card UID.
 * @param nt0  First nonce from card.
 * @param nr0  First reader nonce.
 * @param ar0  First reader response.
 * @param nt1  Second nonce from card.
 * @param nr1  Second reader nonce.
 * @param ar1  Second reader response.
 * @param[out] key  Recovered 48-bit key.
 * @return true if key was recovered successfully.
 */
bool mfkey32(uint32_t uid,
             uint32_t nt0,
             uint32_t nr0,
             uint32_t ar0,
             uint32_t nt1,
             uint32_t nr1,
             uint32_t ar1,
             uint64_t *key);

#ifdef __cplusplus
}
#endif

#endif /* MFKEY_H */
