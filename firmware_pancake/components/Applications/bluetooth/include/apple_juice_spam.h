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

#ifndef APPLE_JUICE_SPAM_H
#define APPLE_JUICE_SPAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Generate an Apple BLE proximity pairing spam payload.
 *
 * @param buffer  Output buffer for the payload. Must not be NULL.
 * @param max_len Maximum size of the output buffer.
 * @return Number of bytes written, or 0 on failure.
 */
int generate_apple_juice_payload(uint8_t *buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // APPLE_JUICE_SPAM_H
