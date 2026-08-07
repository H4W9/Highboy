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

#ifndef SUBGHZ_ANALYZER_H
#define SUBGHZ_ANALYZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result of Sub-GHz signal analysis.
 */
typedef struct {
  uint32_t estimated_te;       /**< @brief Base time element in microseconds. */
  uint32_t pulse_min;          /**< @brief Minimum observed pulse width. */
  uint32_t pulse_max;          /**< @brief Maximum observed pulse width. */
  size_t pulse_count;          /**< @brief Total number of pulses analyzed. */
  const char *modulation_hint; /**< @brief Modulation guess: "PWM", "Manchester", or "Unknown". */
  uint8_t bitstream[128];      /**< @brief Buffer for expected bits. */
  size_t bitstream_len;        /**< @brief Number of recovered bits. */
} subghz_analyzer_result_t;

/**
 * @brief Analyze raw pulse data and extract signal characteristics.
 *
 * @param pulses      Array of signed pulse durations in microseconds.
 * @param count       Number of elements in the pulses array.
 * @param out_result  Pointer to store the analysis result.
 * @return true if analysis succeeded, false otherwise.
 */
bool subghz_analyzer_process(const int32_t *pulses,
                             size_t count,
                             subghz_analyzer_result_t *out_result);

#ifdef __cplusplus
}
#endif

#endif // SUBGHZ_ANALYZER_H
