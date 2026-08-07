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

#ifndef OUI_LOOKUP_H
#define OUI_LOOKUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Look up the vendor name for a MAC address using the OUI prefix.
 *
 * @param mac Pointer to the MAC address (at least 3 bytes). Must not be NULL.
 * @return Vendor name string, or "Unknown" if not found.
 */
const char *oui_get_vendor(const uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif // OUI_LOOKUP_H
