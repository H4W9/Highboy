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

#ifndef MT_MOD_NEIGHBORINFO_H
#define MT_MOD_NEIGHBORINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "mt_modules.h"

void mt_mod_neighborinfo_init(uint32_t node_num);

void mt_mod_neighborinfo_on_received(const mt_packet_meta_t *meta,
                                     const uint8_t *payload,
                                     uint16_t len);

void mt_mod_neighborinfo_tick(uint32_t now_s);

#ifdef __cplusplus
}
#endif

#endif // MT_MOD_NEIGHBORINFO_H
