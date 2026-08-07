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
 * @file snep.h
 * @brief SNEP (Simple NDEF Exchange Protocol) client helpers.
 */
#ifndef SNEP_H
#define SNEP_H

#include <stdint.h>
#include <stddef.h>
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"
#include "llcp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SNEP_VERSION_1_0 0x10U

#define SNEP_REQ_GET 0x01U
#define SNEP_REQ_PUT 0x02U

#define SNEP_RES_SUCCESS         0x81U
#define SNEP_RES_CONTINUE        0x80U
#define SNEP_RES_NOT_FOUND       0xC0U
#define SNEP_RES_EXCESS_DATA     0xC1U
#define SNEP_RES_BAD_REQUEST     0xC2U
#define SNEP_RES_NOT_IMPLEMENTED 0xE0U
#define SNEP_RES_UNSUPPORTED_VER 0xE1U
#define SNEP_RES_REJECT          0xFFU

/**
 * @brief Return a human-readable string for a SNEP response code.
 *
 * @param code  SNEP response code byte.
 * @return Static string describing the response code.
 */
const char *snep_resp_str(uint8_t code);

/**
 * @brief Send an NDEF message to a SNEP server via PUT request.
 *
 * @param link        Active LLCP link context.
 * @param ndef        NDEF message bytes.
 * @param ndef_len    Length of NDEF message.
 * @param[out] resp_code  Server response code on success.
 * @param timeout_ms  Timeout in milliseconds.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_TIMEOUT if server does not respond
 */
hb_nfc_err_t snep_client_put(
    llcp_link_t *link, const uint8_t *ndef, size_t ndef_len, uint8_t *resp_code, int timeout_ms);

/**
 * @brief Send a SNEP GET request and receive the NDEF response.
 *
 * @param link            Active LLCP link context.
 * @param req_ndef        NDEF message describing the request.
 * @param req_len         Length of request NDEF.
 * @param[out] out        Output buffer for response NDEF.
 * @param out_max         Output buffer capacity.
 * @param[out] out_len    Set to actual response length on success.
 * @param[out] resp_code  Server response code.
 * @param timeout_ms      Timeout in milliseconds.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_TIMEOUT if server does not respond
 */
hb_nfc_err_t snep_client_get(llcp_link_t *link,
                             const uint8_t *req_ndef,
                             size_t req_len,
                             uint8_t *out,
                             size_t out_max,
                             size_t *out_len,
                             uint8_t *resp_code,
                             int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* SNEP_H */
