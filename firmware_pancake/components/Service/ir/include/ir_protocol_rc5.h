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

#ifndef IR_PROTOCOL_RC5_H
#define IR_PROTOCOL_RC5_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_protocol.h"

/** @brief RC5 biphase unit duration in microseconds. */
#define RC5_UNIT 889

/** @brief Size of the flat pulse duration buffer used during decode. */
#define RC5_FLAT_BUF_SIZE 64

/** @brief Maximum number of entries consumed from the flat buffer. */
#define RC5_FLAT_BUF_MAX 62

/** @brief Number of data bits decoded from an RC5 frame (excludes start bit). */
#define RC5_DECODE_BITS 13

/** @brief Total number of bits encoded in an RC5 frame (includes start and field bits). */
#define RC5_ENCODE_BITS 14

/** @brief Minimum number of successfully decoded bits to accept a frame. */
#define RC5_MIN_DECODED_BITS 11

/** @brief Bitmask for the 6-bit RC5 command field. */
#define RC5_CMD_MASK 0x3F

/** @brief Bitmask for the 5-bit RC5 address field. */
#define RC5_ADDR_MASK 0x1F

/** @brief Extended command bit set when the RC5-X field bit is 0. */
#define RC5X_CMD_BIT 0x40

/** @brief Bit position of the address field in the RC5 frame word. */
#define RC5_ADDR_SHIFT 6

/**
 * @brief Decode an RC5 or RC5-X IR frame from RMT symbols.
 *
 * Flattens the symbol stream and decodes biphase-space encoded bits.
 * Sets RC5X_CMD_BIT in out_data->command if the RC5-X field bit is absent.
 *
 * @param[in]  symbols   RMT symbol buffer. Must not be NULL.
 * @param[in]  count     Number of symbols. Must be greater than 0.
 * @param[out] out_data  Destination for decoded data. Must not be NULL.
 *
 * @return true if a valid RC5 frame was decoded, false otherwise.
 */
bool ir_protocol_rc5_decode(const rmt_symbol_word_t *symbols, size_t count, ir_data_t *out_data);

/**
 * @brief Encode an RC5 or RC5-X IR command into RMT symbols.
 *
 * Alternates the toggle bit on each call to signal a new keypress.
 * Sets the field bit based on whether the command exceeds RC5_CMD_MASK.
 *
 * @warning Not thread-safe. Must be called from a single task.
 *
 * @param[in]  data     IR command to encode. Must not be NULL.
 * @param[out] symbols  Destination buffer. Must not be NULL.
 * @param[in]  max      Capacity of @p symbols. Must be at least RC5_FLAT_BUF_SIZE.
 *
 * @return Number of symbols written, or 0 on failure.
 */
size_t ir_protocol_rc5_encode(const ir_data_t *data, rmt_symbol_word_t *symbols, size_t max);

#ifdef __cplusplus
}
#endif

#endif // IR_PROTOCOL_RC5_H