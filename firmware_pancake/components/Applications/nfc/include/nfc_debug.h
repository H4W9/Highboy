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

#ifndef NFC_DEBUG_H
#define NFC_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

/**
 * @brief Enable continuous wave (CW) output.
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_COMM on communication failure
 */
hb_nfc_err_t nfc_debug_cw_on(void);

/**
 * @brief Disable continuous wave (CW) output.
 */
void nfc_debug_cw_off(void);

/**
 * @brief Dump all ST25R3916 registers to log.
 */
void nfc_debug_dump_regs(void);

/**
 * @brief Run AAT (Automatic Antenna Tuning) sweep.
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_COMM on communication failure
 *   - HB_NFC_ERR_INTERNAL on hardware error
 */
hb_nfc_err_t nfc_debug_aat_sweep(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_DEBUG_H */
