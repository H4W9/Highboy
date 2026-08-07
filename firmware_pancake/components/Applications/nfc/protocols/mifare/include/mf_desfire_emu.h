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
 * @file mf_desfire_emu.h
 * @brief Minimal MIFARE DESFire (native) APDU emulation helpers.
 */
#ifndef MF_DESFIRE_EMU_H
#define MF_DESFIRE_EMU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset DESFire emulation session state.
 */
void mf_desfire_emu_reset(void);

/**
 * @brief Handle a DESFire native APDU (CLA=0x90).
 *
 * @param apdu      Input APDU buffer.
 * @param apdu_len  Input APDU length.
 * @param[out] out      Output buffer (data + SW1 SW2).
 * @param[out] out_len  Output length.
 * @return true if handled, false if not a DESFire APDU.
 */
bool mf_desfire_emu_handle_apdu(const uint8_t *apdu, int apdu_len, uint8_t *out, int *out_len);

#ifdef __cplusplus
}
#endif

#endif /* MF_DESFIRE_EMU_H */
