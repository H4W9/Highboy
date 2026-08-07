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

#ifndef YS_RFID2_PARSER_H
#define YS_RFID2_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "ys_rfid2_types.h"

/**
 * @brief Reset the parser state, discarding any accumulated bytes.
 */
void ys_rfid2_parser_reset(void);

/**
 * @brief Feed a byte into the parser.
 *
 * Accumulates bytes until the '@' delimiter is found, then attempts to
 * extract a valid card ID from the "card number: xxxxxxxxxx@" frame.
 *
 * @param      byte     Byte received from UART.
 * @param[out] out_raw  Filled with card data if a valid frame was parsed.
 * @return true if a valid card ID was extracted, false otherwise.
 */
bool ys_rfid2_parser_feed(uint8_t byte, ys_rfid2_raw_data_t *out_raw);

#ifdef __cplusplus
}
#endif

#endif // YS_RFID2_PARSER_H
