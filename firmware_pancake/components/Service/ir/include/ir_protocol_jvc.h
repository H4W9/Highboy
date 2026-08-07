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

#ifndef IR_PROTOCOL_JVC_H
#define IR_PROTOCOL_JVC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_protocol.h"

/** @brief JVC header mark duration in microseconds. */
#define JVC_HEADER_MARK 8400

/** @brief JVC header space duration in microseconds. */
#define JVC_HEADER_SPACE 4200

/** @brief JVC bit mark duration in microseconds. */
#define JVC_BIT_MARK 526

/** @brief JVC one-bit space duration in microseconds. */
#define JVC_ONE_SPACE 1578

/** @brief JVC zero-bit space duration in microseconds. */
#define JVC_ZERO_SPACE 526

/** @brief Number of data bits in a JVC frame. */
#define JVC_FRAME_BITS 16

/** @brief Minimum number of RMT symbols for a full JVC frame (with header). */
#define JVC_MIN_SYMBOLS_FULL 18

/** @brief Minimum number of RMT symbols for a JVC repeat frame (no header). */
#define JVC_MIN_SYMBOLS_REPEAT 17

/** @brief Maximum number of RMT symbols for a JVC repeat frame. */
#define JVC_MAX_SYMBOLS_REPEAT 18

/** @brief Number of leading symbols validated to confirm a repeat frame. */
#define JVC_REPEAT_VALIDATE_COUNT 3

/** @brief Bit position of the command field in the JVC frame word. */
#define JVC_CMD_SHIFT 8

/** @brief Bit mask for the 8-bit address field in the JVC frame. */
#define JVC_ADDR_MASK 0xFF

/**
 * @brief Decode a JVC IR frame from RMT symbols.
 *
 * Accepts both full frames (with header) and repeat frames (no header).
 * Sets out_data->repeat to true if no header was detected.
 *
 * @param[in]  symbols   RMT symbol buffer. Must not be NULL.
 * @param[in]  count     Number of symbols. Must be greater than 0.
 * @param[out] out_data  Destination for decoded data. Must not be NULL.
 *
 * @return true if a valid JVC frame was decoded, false otherwise.
 */
bool ir_protocol_jvc_decode(const rmt_symbol_word_t *symbols, size_t count, ir_data_t *out_data);

/**
 * @brief Encode a JVC IR command into RMT symbols.
 *
 * If data->repeat is true, the header is omitted from the encoded frame.
 *
 * @param[in]  data     IR command to encode. Must not be NULL.
 * @param[out] symbols  Destination buffer. Must not be NULL.
 * @param[in]  max      Capacity of @p symbols in symbols.
 *
 * @return Number of symbols written, or 0 on failure.
 */
size_t ir_protocol_jvc_encode(const ir_data_t *data, rmt_symbol_word_t *symbols, size_t max);

#ifdef __cplusplus
}
#endif

#endif // IR_PROTOCOL_JVC_H