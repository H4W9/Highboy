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

#ifndef RFID_PROTOCOL_DECODER_H
#define RFID_PROTOCOL_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ys_rfid2_types.h"
#include "rfid_types.h"

/**
 * @brief Interface for RFID 125kHz protocol decoders.
 */
typedef struct {
  const char *name;

  /**
   * @brief Decode raw card data into structured fields.
   *
   * @param raw       Raw data from the RFID reader (40-bit card ID).
   * @param out_data  Pointer to store decoded results.
   * @return true if the protocol recognized the data.
   */
  bool (*decode)(const ys_rfid2_raw_data_t *raw, rfid_decoded_data_t *out_data);
} rfid_protocol_t;

#ifdef __cplusplus
}
#endif

#endif // RFID_PROTOCOL_DECODER_H
