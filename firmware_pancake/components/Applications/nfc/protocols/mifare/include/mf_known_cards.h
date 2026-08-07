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
/**
 * @file mf_known_cards.h
 * @brief Known MIFARE Classic card type database (name + identification only).
 *
 * Matches a card by SAK + ATQA (and optionally UID prefix) and returns a
 * human-readable name plus a hint string. Key dictionaries are no longer
 * tied to card profiles — the brute-force loop in the reader iterates
 * @ref mf_key_dict, which already aggregates every mf_classic_*.dic file
 * the user dropped in /sdcard/nfc/assets/dict/.
 */
#ifndef MF_KNOWN_CARDS_H
#define MF_KNOWN_CARDS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Known card database entry.
 */
typedef struct {
  const char *name;
  const char *hint;
  uint8_t sak;
  uint8_t atqa[2];
  uint8_t uid_prefix[4];
  uint8_t uid_prefix_len; /**< @brief 0 = do not check UID prefix. */
} mf_known_card_t;

/**
 * @brief Try to match a card against the known cards database.
 *
 * @param sak      SAK byte.
 * @param atqa     2-byte ATQA.
 * @param uid      Card UID.
 * @param uid_len  UID length in bytes.
 * @return Pointer to matched entry, or NULL if unknown.
 */
const mf_known_card_t *
mf_known_cards_match(uint8_t sak, const uint8_t atqa[2], const uint8_t *uid, uint8_t uid_len);

#ifdef __cplusplus
}
#endif
#endif /* MF_KNOWN_CARDS_H */
