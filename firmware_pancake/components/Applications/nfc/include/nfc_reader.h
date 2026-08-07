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

#ifndef NFC_READER_H
#define NFC_READER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "highboy_nfc_types.h"
#include "highboy_nfc_compat.h"
#include "mf_classic_emu.h"

/** @brief Global emulation card data populated by mf_classic_read_full(). */
extern mfc_emu_card_data_t s_emu_card;

/** @brief true when s_emu_card contains valid data from a completed read. */
extern bool s_is_emu_data_ready;

/**
 * @brief Read all sectors of a MIFARE Classic card.
 *
 * @param card Card data with UID/SAK/ATQA.
 */
void mf_classic_read_full(nfc_iso14443a_data_t *card);

/**
 * @brief Write all sectors from s_emu_card back to a target card.
 *
 * Requires s_is_emu_data_ready == true (i.e. mf_classic_read_full completed).
 * Uses the keys stored in s_emu_card for authentication.
 * Data blocks only - trailer write is skipped unless is_write_trailers is true.
 * WARNING: writing trailers with wrong access bits can permanently lock sectors.
 *
 * @param target            Target card data.
 * @param is_write_trailers Whether to write trailer blocks.
 */
void mf_classic_write_all(nfc_iso14443a_data_t *target, bool is_write_trailers);

/**
 * @brief Probe and dump MIFARE Plus card.
 *
 * @param card Card data.
 */
void mfp_probe_and_dump(nfc_iso14443a_data_t *card);

/**
 * @brief Dump MIFARE Ultralight card.
 *
 * @param card Card data.
 */
void mful_dump_card(nfc_iso14443a_data_t *card);

/**
 * @brief Dump NDEF data from a T4T card.
 *
 * @param card Card data.
 */
void t4t_dump_ndef(nfc_iso14443a_data_t *card);

#ifdef __cplusplus
}
#endif

#endif /* NFC_READER_H */
