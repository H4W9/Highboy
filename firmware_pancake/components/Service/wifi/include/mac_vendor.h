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

#ifndef MAC_VENDOR_H
#define MAC_VENDOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Look up the vendor name for a MAC address.
 *
 * Returns "Randomized" for locally administered MACs (bit 1 of first octet
 * set), or an empty string if the OUI is not in the lookup table.
 *
 * @param mac  Pointer to the 6-byte MAC address.
 * @return Vendor name string. Valid for the lifetime of the program.
 */
const char *mac_vendor_get_name(const uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif // MAC_VENDOR_H
