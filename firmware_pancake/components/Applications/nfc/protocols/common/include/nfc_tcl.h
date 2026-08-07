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

#ifndef NFC_TCL_H
#define NFC_TCL_H

#include <stdint.h>
#include <stddef.h>

#include "highboy_nfc_types.h"
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Transceive data over ISO14443-4 (T=CL) for type A or B.
 *
 * @param proto      HB_PROTO_ISO14443_4A or HB_PROTO_ISO14443_4B.
 * @param ctx        Protocol context (nfc_iso_dep_data_t* for A, nfc_iso14443b_data_t* for B).
 * @param tx         Transmit data buffer.
 * @param tx_len     Length of the transmit data.
 * @param[out] rx        Receive data buffer.
 * @param rx_max     Maximum size of the receive buffer.
 * @param timeout_ms Timeout in milliseconds.
 * @return Number of bytes received, or 0 on failure.
 */
int nfc_tcl_transceive(hb_nfc_protocol_t proto,
                       const void *ctx,
                       const uint8_t *tx,
                       size_t tx_len,
                       uint8_t *rx,
                       size_t rx_max,
                       int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* NFC_TCL_H */
