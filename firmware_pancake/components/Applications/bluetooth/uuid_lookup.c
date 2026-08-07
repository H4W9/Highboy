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

#include "uuid_lookup.h"

#include <stddef.h>

typedef struct {
  uint16_t uuid16;
  const char *name;
} uuid_lookup_entry_t;

static const uuid_lookup_entry_t UUID_TABLE[] = {
    {0x1800, "Generic Access"},
    {0x1801, "Generic Attribute"},
    {0x180A, "Device Information"},
    {0x180D, "Heart Rate"},
    {0x180F, "Battery Service"},
    {0x1812, "Human Interface Device"},
    {0x181A, "Environmental Sensing"},
    {0x2A00, "Device Name"},
    {0x2A01, "Appearance"},
    {0x2A19, "Battery Level"},
    {0x2A29, "Manufacturer Name"},
    {0x2A37, "Heart Rate Measurement"},
    {0x2A4D, "Report"},
};
#define UUID_TABLE_COUNT (sizeof(UUID_TABLE) / sizeof(UUID_TABLE[0]))

const char *uuid_get_name_by_u16(uint16_t uuid16) {
  for (size_t i = 0; i < UUID_TABLE_COUNT; i++) {
    if (UUID_TABLE[i].uuid16 == uuid16) {
      return UUID_TABLE[i].name;
    }
  }
  return "Unknown Service/Char";
}
