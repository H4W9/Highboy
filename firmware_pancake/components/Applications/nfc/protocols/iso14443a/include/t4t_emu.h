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
 * @file t4t_emu.h
 * @brief ISO14443-4 / T4T emulation (ISO-DEP target).
 */
#ifndef T4T_EMU_H
#define T4T_EMU_H

#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize default NDEF and CC data for emulation.
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_NO_MEM on allocation failure
 */
hb_nfc_err_t t4t_emu_init_default(void);

/**
 * @brief Configure ST25R3916 as NFC-A target for ISO-DEP.
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_IO on hardware failure
 */
hb_nfc_err_t t4t_emu_configure_target(void);

/**
 * @brief Start emulation and enter SLEEP state.
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_IO on hardware failure
 */
hb_nfc_err_t t4t_emu_start(void);

/**
 * @brief Stop emulation.
 */
void t4t_emu_stop(void);

/**
 * @brief Run one emulation step (call in tight loop).
 */
void t4t_emu_run_step(void);

#ifdef __cplusplus
}
#endif

#endif /* T4T_EMU_H */
