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

#ifndef EMU_DIAG_H
#define EMU_DIAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

/**
 * @brief Run full target mode diagnostic.
 *
 * Tests field detection, multiple configs, PT Memory, oscillator.
 * Takes ~60 seconds. Share the FULL serial output!
 *
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_COMM on communication failure
 *   - HB_NFC_ERR_INTERNAL on hardware error
 */
hb_nfc_err_t emu_diag_full(void);

/**
 * @brief Monitor target interrupts for N seconds.
 *
 * @param seconds Number of seconds to monitor.
 */
void emu_diag_monitor(int seconds);

#ifdef __cplusplus
}
#endif

#endif /* EMU_DIAG_H */
