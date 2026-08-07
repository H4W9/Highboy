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

#include "nfc_tcl.h"

#include "esp_log.h"

#include "iso_dep.h"
#include "iso14443b.h"

static const char *TAG = "NFC_TCL";

int nfc_tcl_transceive(hb_nfc_protocol_t proto,
                       const void *ctx,
                       const uint8_t *tx,
                       size_t tx_len,
                       uint8_t *rx,
                       size_t rx_max,
                       int timeout_ms) {
  if (!ctx || !tx || !rx || tx_len == 0)
    return 0;

  if (proto == HB_PROTO_ISO14443_4A) {
    return iso_dep_transceive((const nfc_iso_dep_data_t *)ctx, tx, tx_len, rx, rx_max, timeout_ms);
  }
  if (proto == HB_PROTO_ISO14443_4B) {
    return iso14443b_tcl_transceive(
        (const nfc_iso14443b_data_t *)ctx, tx, tx_len, rx, rx_max, timeout_ms);
  }

  return 0;
}
