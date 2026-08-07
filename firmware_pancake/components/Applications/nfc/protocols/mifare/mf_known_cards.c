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

#include "mf_known_cards.h"

#include <string.h>
#include <stddef.h>

static const mf_known_card_t s_db[] = {
    {
        .name = "NXP MIFARE Classic 1K",
        .hint = "Genuine NXP (cascade byte 0x04)",
        .sak = 0x08,
        .atqa = {0x00, 0x04},
        .uid_prefix = {0x04},
        .uid_prefix_len = 1,
    },
    {
        .name = "NXP MIFARE Classic 4K",
        .hint = "Genuine NXP (cascade byte 0x04)",
        .sak = 0x18,
        .atqa = {0x00, 0x02},
        .uid_prefix = {0x04},
        .uid_prefix_len = 1,
    },
    {
        .name = "MIFARE Classic 1K Clone (Gen2/CUID)",
        .hint = "UID changeable, writable block 0",
        .sak = 0x08,
        .atqa = {0x00, 0x44},
    },
    {
        .name = "MIFARE Classic 4K Clone",
        .hint = "UID changeable clone",
        .sak = 0x18,
        .atqa = {0x00, 0x42},
    },
    {
        .name = "Infineon MIFARE Classic 1K",
        .hint = "SLE66R35, used in older access/transit systems",
        .sak = 0x88,
        .atqa = {0x00, 0x04},
    },
    {
        .name = "NXP SmartMX MIFARE Classic 1K",
        .hint = "P60/P40 combo chip, SAK=0x28",
        .sak = 0x28,
        .atqa = {0x00, 0x04},
    },
    {
        .name = "NXP SmartMX MIFARE Classic 4K",
        .hint = "P60/P40 combo chip, SAK=0x38",
        .sak = 0x38,
        .atqa = {0x00, 0x02},
    },
    {
        .name = "Transit/Ticketing Card (1K)",
        .hint = "Common for metro/bus access systems",
        .sak = 0x08,
        .atqa = {0x00, 0x04},
    },
    {
        .name = "Transit/Ticketing Card (4K)",
        .hint = "Large-memory transit card",
        .sak = 0x18,
        .atqa = {0x00, 0x02},
    },
    {
        .name = "Access Control Card (1K)",
        .hint = "Building/door access system",
        .sak = 0x08,
        .atqa = {0x08, 0x00},
    },
    {
        .name = "MIFARE Mini",
        .hint = "320 bytes / 5 sectors",
        .sak = 0x09,
        .atqa = {0x00, 0x04},
    },
};

#define DB_SIZE ((int)(sizeof(s_db) / sizeof(s_db[0])))

const mf_known_card_t *
mf_known_cards_match(uint8_t sak, const uint8_t atqa[2], const uint8_t *uid, uint8_t uid_len) {
  const mf_known_card_t *generic_match = NULL;

  for (int i = 0; i < DB_SIZE; i++) {
    const mf_known_card_t *e = &s_db[i];

    if (e->sak != sak)
      continue;
    if (e->atqa[0] != atqa[0] || e->atqa[1] != atqa[1])
      continue;

    if (e->uid_prefix_len > 0) {
      if (uid_len < e->uid_prefix_len)
        continue;
      if (memcmp(uid, e->uid_prefix, e->uid_prefix_len) != 0)
        continue;
      return e;
    } else {
      if (generic_match == NULL)
        generic_match = e;
    }
  }

  return generic_match;
}
