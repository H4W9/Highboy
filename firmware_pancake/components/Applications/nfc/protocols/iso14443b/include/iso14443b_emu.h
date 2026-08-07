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
 * @file iso14443b_emu.h
 * @brief ISO14443B (NFC-B) target emulation with basic ISO-DEP/T4T APDUs.
 */
#ifndef ISO14443B_EMU_H
#define ISO14443B_EMU_H

#include <stdint.h>
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NFC-B emulation card profile.
 */
typedef struct {
  uint8_t pupi[4];      /**< Pseudo-Unique PICC Identifier. */
  uint8_t app_data[4];  /**< Application Data from ATQB. */
  uint8_t prot_info[3]; /**< Protocol Info from ATQB. */
} iso14443b_emu_card_t;

/**
 * @brief Initialize default ATQB and T4T NDEF payload.
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_IO on communication failure
 */
hb_nfc_err_t iso14443b_emu_init_default(void);

/**
 * @brief Configure ST25R3916 as NFC-B target (ISO14443-3B).
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_IO on communication failure
 */
hb_nfc_err_t iso14443b_emu_configure_target(void);

/**
 * @brief Start emulation (enter SLEEP state).
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_IO on communication failure
 */
hb_nfc_err_t iso14443b_emu_start(void);

/**
 * @brief Stop emulation and power down.
 */
void iso14443b_emu_stop(void);

/**
 * @brief Run one emulation step (call in tight loop).
 */
void iso14443b_emu_run_step(void);

#ifdef __cplusplus
}
#endif

#endif /* ISO14443B_EMU_H */
