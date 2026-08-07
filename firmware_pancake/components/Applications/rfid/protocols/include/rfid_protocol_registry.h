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

#ifndef RFID_PROTOCOL_REGISTRY_H
#define RFID_PROTOCOL_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rfid_protocol_decoder.h"

/**
 * @brief Initialize the protocol registry.
 */
void rfid_protocol_registry_init(void);

/**
 * @brief Run all registered decoders on raw card data.
 *
 * @param raw       Raw data from the RFID reader.
 * @param out_data  Pointer to store decoded results.
 * @return true if a protocol claimed the data.
 */
bool rfid_protocol_registry_decode_all(const ys_rfid2_raw_data_t *raw,
                                       rfid_decoded_data_t *out_data);

/**
 * @brief Find a protocol by name.
 *
 * @param name  Protocol name to search for.
 * @return Pointer to protocol structure, or NULL if not found.
 */
const rfid_protocol_t *rfid_protocol_registry_get_by_name(const char *name);

/**
 * @brief Get the number of registered protocols.
 */
size_t rfid_protocol_registry_get_count(void);

/**
 * @brief Get a protocol by index.
 *
 * @param index  Zero-based index into the registry.
 * @return Pointer to protocol structure, or NULL if index is out of range.
 */
const rfid_protocol_t *rfid_protocol_registry_get_by_index(size_t index);

#ifdef __cplusplus
}
#endif

#endif // RFID_PROTOCOL_REGISTRY_H
