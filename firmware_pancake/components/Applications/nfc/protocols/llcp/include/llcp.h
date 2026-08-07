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
 * @file llcp.h
 * @brief LLCP (Logical Link Control Protocol) over NFC-DEP.
 */
#ifndef LLCP_H
#define LLCP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LLCP_MAGIC_0 0x46U
#define LLCP_MAGIC_1 0x66U
#define LLCP_MAGIC_2 0x6DU

/* LLCP PTYPE values */
#define LLCP_PTYPE_SYMM    0x0U
#define LLCP_PTYPE_PAX     0x1U
#define LLCP_PTYPE_AGF     0x2U
#define LLCP_PTYPE_UI      0x3U
#define LLCP_PTYPE_CONNECT 0x4U
#define LLCP_PTYPE_DISC    0x5U
#define LLCP_PTYPE_CC      0x6U
#define LLCP_PTYPE_DM      0x7U
#define LLCP_PTYPE_FRMR    0x8U
#define LLCP_PTYPE_SNL     0x9U
#define LLCP_PTYPE_I       0xCU
#define LLCP_PTYPE_RR      0xDU
#define LLCP_PTYPE_RNR     0xEU

/* LLCP TLV types */
#define LLCP_TLV_VERSION 0x01U
#define LLCP_TLV_MIUX    0x02U
#define LLCP_TLV_WKS     0x03U
#define LLCP_TLV_LTO     0x04U
#define LLCP_TLV_RW      0x05U
#define LLCP_TLV_SN      0x06U
#define LLCP_TLV_OPT     0x07U

/**
 * @brief LLCP link parameters.
 */
typedef struct {
  uint8_t version_major; /**< LLCP major version number. */
  uint8_t version_minor; /**< LLCP minor version number. */
  uint16_t miu;          /**< Maximum Information Unit. */
  uint16_t wks;          /**< Well-Known Services bitmap. */
  uint16_t lto_ms;       /**< Link Timeout in milliseconds. */
  uint8_t rw;            /**< Receive window size. */
} llcp_params_t;

/**
 * @brief LLCP link state context.
 */
typedef struct {
  uint8_t nfcid3[10];       /**< NFC-DEP Identifier (10 bytes). */
  uint8_t did;              /**< Device Identifier. */
  uint8_t bs;               /**< Supported send bit rates. */
  uint8_t br;               /**< Supported receive bit rates. */
  uint8_t pp;               /**< Protocol Parameters byte. */
  uint8_t to;               /**< Timeout value. */
  uint16_t lr;              /**< Length Reduction value. */
  uint16_t rwt_ms;          /**< Response Waiting Time in ms. */
  uint8_t pni;              /**< Packet Number Information. */
  llcp_params_t local;      /**< Local link parameters. */
  llcp_params_t remote;     /**< Remote link parameters. */
  llcp_params_t negotiated; /**< Negotiated link parameters. */
  bool llcp_active;         /**< true if LLCP link is active. */
} llcp_link_t;

/**
 * @brief Fill LLCP parameters with default values.
 *
 * @param[out] params  Parameters to initialise.
 */
void llcp_params_default(llcp_params_t *params);

/**
 * @brief Initialise an LLCP link context to defaults.
 *
 * @param[out] link  Link context to initialise.
 */
void llcp_link_init(llcp_link_t *link);

/**
 * @brief Build General Bytes (GT) TLV payload from local parameters.
 *
 * @param params  Local LLCP parameters.
 * @param[out] out  Output buffer for GT bytes.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_gt(const llcp_params_t *params, uint8_t *out, size_t max);

/**
 * @brief Parse General Bytes (GT) TLV payload into parameters.
 *
 * @param gt       Input GT bytes.
 * @param gt_len   Length of GT bytes.
 * @param[out] out Parsed parameters.
 * @return true on success, false on parse error.
 */
bool llcp_parse_gt(const uint8_t *gt, size_t gt_len, llcp_params_t *out);

/**
 * @brief Activate LLCP as initiator over NFC-DEP.
 *
 * @param link  Link context (local params must be set).
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_TIMEOUT if target does not respond
 */
hb_nfc_err_t llcp_initiator_activate(llcp_link_t *link);

/**
 * @brief Exchange a single LLCP PDU (send TX, receive RX).
 *
 * @param link        Active LLCP link context.
 * @param tx_pdu      PDU bytes to send.
 * @param tx_len      Length of TX PDU.
 * @param[out] rx_pdu Receive buffer for response PDU.
 * @param rx_max      Receive buffer capacity.
 * @param timeout_ms  Timeout in milliseconds.
 * @return Number of bytes received, or negative on error.
 */
int llcp_exchange_pdu(llcp_link_t *link,
                      const uint8_t *tx_pdu,
                      size_t tx_len,
                      uint8_t *rx_pdu,
                      size_t rx_max,
                      int timeout_ms);

/**
 * @brief Build an LLCP PDU header (DSAP/PTYPE/SSAP).
 *
 * @param dsap   Destination Service Access Point.
 * @param ptype  PDU type.
 * @param ssap   Source Service Access Point.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_header(uint8_t dsap, uint8_t ptype, uint8_t ssap, uint8_t *out, size_t max);

/**
 * @brief Parse an LLCP PDU header.
 *
 * @param pdu          Input PDU buffer.
 * @param len          PDU length.
 * @param[out] dsap    Destination SAP.
 * @param[out] ptype   PDU type.
 * @param[out] ssap    Source SAP.
 * @param[out] ns      Send sequence number (I-PDU only).
 * @param[out] nr      Receive sequence number (I/RR/RNR only).
 * @return true on success, false if PDU is too short.
 */
bool llcp_parse_header(const uint8_t *pdu,
                       size_t len,
                       uint8_t *dsap,
                       uint8_t *ptype,
                       uint8_t *ssap,
                       uint8_t *ns,
                       uint8_t *nr);

/**
 * @brief Build a SYMM (keep-alive) PDU.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_symm(uint8_t *out, size_t max);

/**
 * @brief Build a UI (Unnumbered Information) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param info      Information payload.
 * @param info_len  Length of info payload.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_ui(
    uint8_t dsap, uint8_t ssap, const uint8_t *info, size_t info_len, uint8_t *out, size_t max);

/**
 * @brief Build a CONNECT PDU with optional service name and parameters.
 *
 * @param dsap          Destination SAP.
 * @param ssap          Source SAP.
 * @param service_name  Service name string (NULL if not needed).
 * @param params        Connection parameters (NULL for defaults).
 * @param[out] out      Output buffer.
 * @param max           Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_connect(uint8_t dsap,
                          uint8_t ssap,
                          const char *service_name,
                          const llcp_params_t *params,
                          uint8_t *out,
                          size_t max);

/**
 * @brief Build a CC (Connection Complete) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param params    Connection parameters (NULL for defaults).
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t
llcp_build_cc(uint8_t dsap, uint8_t ssap, const llcp_params_t *params, uint8_t *out, size_t max);

/**
 * @brief Build a DISC (Disconnect) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_disc(uint8_t dsap, uint8_t ssap, uint8_t *out, size_t max);

/**
 * @brief Build a DM (Disconnected Mode) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param reason    Disconnect reason code.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_dm(uint8_t dsap, uint8_t ssap, uint8_t reason, uint8_t *out, size_t max);

/**
 * @brief Build an I (Information) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param ns        Send sequence number.
 * @param nr        Receive sequence number.
 * @param info      Information payload.
 * @param info_len  Length of info payload.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_i(uint8_t dsap,
                    uint8_t ssap,
                    uint8_t ns,
                    uint8_t nr,
                    const uint8_t *info,
                    size_t info_len,
                    uint8_t *out,
                    size_t max);

/**
 * @brief Build an RR (Receive Ready) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param nr        Receive sequence number.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_rr(uint8_t dsap, uint8_t ssap, uint8_t nr, uint8_t *out, size_t max);

/**
 * @brief Build an RNR (Receive Not Ready) PDU.
 *
 * @param dsap      Destination SAP.
 * @param ssap      Source SAP.
 * @param nr        Receive sequence number.
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @return Number of bytes written.
 */
size_t llcp_build_rnr(uint8_t dsap, uint8_t ssap, uint8_t nr, uint8_t *out, size_t max);

/**
 * @brief Encode a VERSION TLV.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @param version   Version byte (major << 4 | minor).
 * @return Number of bytes written.
 */
size_t llcp_tlv_version(uint8_t *out, size_t max, uint8_t version);

/**
 * @brief Encode a MIUX TLV.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @param miux      MIUX value (MIU extension).
 * @return Number of bytes written.
 */
size_t llcp_tlv_miux(uint8_t *out, size_t max, uint16_t miux);

/**
 * @brief Encode a WKS (Well-Known Services) TLV.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @param wks       Well-Known Services bitmap.
 * @return Number of bytes written.
 */
size_t llcp_tlv_wks(uint8_t *out, size_t max, uint16_t wks);

/**
 * @brief Encode an LTO (Link Timeout) TLV.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @param lto       Link Timeout value (units of 10 ms).
 * @return Number of bytes written.
 */
size_t llcp_tlv_lto(uint8_t *out, size_t max, uint8_t lto);

/**
 * @brief Encode an RW (Receive Window) TLV.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @param rw        Receive Window size.
 * @return Number of bytes written.
 */
size_t llcp_tlv_rw(uint8_t *out, size_t max, uint8_t rw);

/**
 * @brief Encode an SN (Service Name) TLV.
 *
 * @param[out] out  Output buffer.
 * @param max       Output buffer capacity.
 * @param sn        Null-terminated service name string.
 * @return Number of bytes written.
 */
size_t llcp_tlv_sn(uint8_t *out, size_t max, const char *sn);

#ifdef __cplusplus
}
#endif

#endif /* LLCP_H */
