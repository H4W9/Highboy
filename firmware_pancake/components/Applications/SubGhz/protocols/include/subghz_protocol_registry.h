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

#ifndef SUBGHZ_PROTOCOL_REGISTRY_H
#define SUBGHZ_PROTOCOL_REGISTRY_H

#include "subghz_protocol_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the protocol registry.
 */
void subghz_protocol_registry_init(void);

/**
 * @brief Run all registered decoders on the signal.
 *
 * @param pulses    Array of signed pulse durations in microseconds.
 * @param count     Number of elements in the pulses array.
 * @param out_data  Pointer to store decoded results.
 * @return true if a protocol claimed the signal.
 */
bool subghz_protocol_registry_decode_all(const int32_t *pulses,
                                         size_t count,
                                         subghz_data_t *out_data);

/**
 * @brief Find a protocol by name.
 *
 * @param name  Protocol name to search for.
 * @return Pointer to protocol structure, or NULL if not found.
 */
const subghz_protocol_t *subghz_protocol_registry_get_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif // SUBGHZ_PROTOCOL_REGISTRY_H
