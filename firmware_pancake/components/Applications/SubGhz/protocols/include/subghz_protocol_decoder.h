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

#ifndef SUBGHZ_PROTOCOL_DECODER_H
#define SUBGHZ_PROTOCOL_DECODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "subghz_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interface for Sub-GHz protocol decoders.
 */
typedef struct {
  const char *name;

  /**
   * @brief Decode raw pulse data into structured subghz_data_t.
   *
   * @param pulses    Array of signed integers (positive = high, negative = low in microseconds).
   * @param count     Number of pulses in the array.
   * @param out_data  Pointer to store decoded results.
   * @return true if signal was recognized and decoded.
   */
  bool (*decode)(const int32_t *pulses, size_t count, subghz_data_t *out_data);

  /**
   * @brief Encode structured data back into pulse timings for transmission.
   *
   * @param data       Data to encode.
   * @param pulses     Output buffer for pulse timings.
   * @param max_count  Size of the pulses buffer.
   * @return Number of pulses written, or 0 on failure.
   */
  size_t (*encode)(const subghz_data_t *data, int32_t *pulses, size_t max_count);
} subghz_protocol_t;

#ifdef __cplusplus
}
#endif

#endif // SUBGHZ_PROTOCOL_DECODER_H
