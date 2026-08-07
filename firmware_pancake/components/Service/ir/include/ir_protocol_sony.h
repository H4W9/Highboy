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

#ifndef IR_PROTOCOL_SONY_H
#define IR_PROTOCOL_SONY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_protocol.h"

/** @brief Sony header mark duration in microseconds. */
#define SONY_HEADER_MARK 2400

/** @brief Sony header space duration in microseconds. */
#define SONY_HEADER_SPACE 600

/** @brief Sony one-bit mark duration in microseconds. */
#define SONY_ONE_MARK 1200

/** @brief Sony zero-bit mark duration in microseconds. */
#define SONY_ZERO_MARK 600

/** @brief Sony inter-bit space duration in microseconds. */
#define SONY_BIT_SPACE 600

/** @brief Minimum RMT symbol count to identify a SIRC-20 frame. */
#define SONY_SIRC20_MIN_SYMBOLS 21

/** @brief Number of data bits in a SIRC-20 frame. */
#define SONY_SIRC20_BITS 20

/** @brief Minimum RMT symbol count to identify a SIRC-15 frame. */
#define SONY_SIRC15_MIN_SYMBOLS 16

/** @brief Number of data bits in a SIRC-15 frame. */
#define SONY_SIRC15_BITS 15

/** @brief Minimum RMT symbol count to identify a SIRC-12 frame. */
#define SONY_SIRC12_MIN_SYMBOLS 13

/** @brief Number of data bits in a SIRC-12 frame. */
#define SONY_SIRC12_BITS 12

/** @brief Maximum address value for a SIRC-12 frame (5-bit device). */
#define SONY_SIRC12_ADDR_MAX 0x1F

/** @brief Maximum address value for a SIRC-15 frame (8-bit device). */
#define SONY_SIRC15_ADDR_MAX 0xFF

/** @brief Bit mask for the 7-bit command field in the SIRC frame. */
#define SONY_CMD_MASK 0x7F

/** @brief Bit position of the address field in the SIRC frame word. */
#define SONY_ADDR_SHIFT 7

/** @brief Bit mask for the 13-bit address field in the SIRC frame. */
#define SONY_ADDR_MASK 0x1FFF

/**
 * @brief Decode a Sony IR frame from RMT symbols.
 *
 * Supports 12-bit, 15-bit, and 20-bit Sony SIRC variants.
 *
 * @param[in]  symbols   RMT symbol buffer. Must not be NULL.
 * @param[in]  count     Number of symbols. Must be greater than 0.
 * @param[out] out_data  Destination for decoded data. Must not be NULL.
 *
 * @return true if a valid Sony frame was decoded, false otherwise.
 */
bool ir_protocol_sony_decode(const rmt_symbol_word_t *symbols, size_t count, ir_data_t *out_data);

/**
 * @brief Encode a Sony IR command into RMT symbols.
 *
 * Selects 12-bit, 15-bit, or 20-bit variant based on the address range.
 *
 * @param[in]  data     IR command to encode. Must not be NULL.
 * @param[out] symbols  Destination buffer. Must not be NULL.
 * @param[in]  max      Capacity of @p symbols in symbols.
 *
 * @return Number of symbols written, or 0 on failure.
 */
size_t ir_protocol_sony_encode(const ir_data_t *data, rmt_symbol_word_t *symbols, size_t max);

#ifdef __cplusplus
}
#endif

#endif // IR_PROTOCOL_SONY_H