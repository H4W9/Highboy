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
 * @file llcp.c
 * @brief NFC-DEP / LLCP initiator: ATR exchange, DEP transceive, and PDU builders.
 * Reference: NFC Forum LLCP 1.3, ISO/IEC 18092 (NFC-DEP).
 */
#include "llcp.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"

#include "nfc_poller.h"
#include "nfc_rf.h"
#include "nfc_common.h"
#include "iso14443a.h"
#include "st25r3916_core.h"

static const char *TAG = "NFC_LLCP";

#define NFCDEP_CMD0_REQ     0xD4U
#define NFCDEP_CMD0_RES     0xD5U
#define NFCDEP_CMD1_ATR_REQ 0x00U
#define NFCDEP_CMD1_ATR_RES 0x01U
#define NFCDEP_CMD1_DEP_REQ 0x06U
#define NFCDEP_CMD1_DEP_RES 0x07U

#define NFCDEP_PFB_PNI_MASK 0x03U
#define NFCDEP_PFB_DID      0x04U
#define NFCDEP_PFB_NAD      0x08U
#define NFCDEP_PFB_MI       0x10U

#define LLCP_NIBBLE_MASK       0x0FU
#define LLCP_BYTE_MASK         0xFFU
#define LLCP_MIUX_MASK         0x07FFU
#define LLCP_SAP_MASK          0x3FU
#define LLCP_PTYPE_LO_MASK     0x03U
#define LLCP_PP_GI_BIT         0x40U
#define LLCP_PNI_MASK          0x03U
#define LLCP_WKS_DEFAULT       0x0013U
#define LLCP_DEFAULT_MIU       128U
#define LLCP_LR_64             64U
#define LLCP_LR_128            128U
#define LLCP_LR_192            192U
#define LLCP_LR_254            254U
#define LLCP_NFCID3_LEN        10
#define LLCP_CRC_LEN           2U
#define LLCP_MAGIC_BYTES       3U
#define LLCP_ATR_RES_MIN_LEN   17
#define LLCP_LTO_UNIT_MS       10U
#define LLCP_DEP_DEFAULT_TMO   50U
#define LLCP_RWT_BASE_US       302U
#define LLCP_DEFAULT_PARAM_MIU 248U
#define LLCP_DEFAULT_LTO_MS    1000U
#define LLCP_DEFAULT_RW        4
#define LLCP_US_PER_MS         1000U
#define LLCP_RWT_MIN_MS        5U

#define LLCP_ATR_RES_DID_OFF 12
#define LLCP_ATR_RES_BS_OFF  13
#define LLCP_ATR_RES_BR_OFF  14
#define LLCP_ATR_RES_TO_OFF  15
#define LLCP_ATR_RES_PP_OFF  16

#define NFCDEP_HDR_SIZE    3
#define NFCDEP_MAX_PAYLOAD 256

static int llcp_strip_crc(uint8_t *buf, int len);
static uint16_t llcp_rwt_ms(uint8_t to);
static uint16_t llcp_lr_from_pp(uint8_t pp);
static uint8_t llcp_pp_from_miu(uint16_t miu);
static void llcp_compute_negotiated(llcp_link_t *link);
static int nfc_dep_transceive(llcp_link_t *link,
                              const uint8_t *tx,
                              size_t tx_len,
                              uint8_t *rx,
                              size_t rx_max,
                              int timeout_ms);

static int llcp_strip_crc(uint8_t *buf, int len) {
  if (len >= (int)(LLCP_CRC_LEN + 1U) && iso14443a_check_crc(buf, (size_t)len))
    return len - (int)LLCP_CRC_LEN;
  return len;
}

static uint16_t llcp_rwt_ms(uint8_t to) {
  uint32_t us = LLCP_RWT_BASE_US * (1U << (to & LLCP_NIBBLE_MASK));
  uint32_t ms = (us + (LLCP_US_PER_MS - 1U)) / LLCP_US_PER_MS;
  if (ms < LLCP_RWT_MIN_MS)
    ms = LLCP_RWT_MIN_MS;
  return (uint16_t)ms;
}

static uint16_t llcp_lr_from_pp(uint8_t pp) {
  if ((pp & LLCP_PP_GI_BIT) == 0)
    return LLCP_LR_64;
  switch ((pp >> 4) & LLCP_PTYPE_LO_MASK) {
    case 0:
      return LLCP_LR_64;
    case 1:
      return LLCP_LR_128;
    case 2:
      return LLCP_LR_192;
    default:
      return LLCP_LR_254;
  }
}

static uint8_t llcp_pp_from_miu(uint16_t miu) {
  uint8_t lr_bits = 0;
  if (miu > LLCP_LR_192)
    lr_bits = 3;
  else if (miu > LLCP_LR_128)
    lr_bits = 2;
  else if (miu > LLCP_LR_64)
    lr_bits = 1;
  else
    lr_bits = 0;
  return (uint8_t)(LLCP_PP_GI_BIT | (lr_bits << 4));
}

void llcp_params_default(llcp_params_t *params) {
  if (params == NULL)
    return;
  params->version_major = 1;
  params->version_minor = 1;
  params->miu = LLCP_DEFAULT_PARAM_MIU;
  params->wks = LLCP_WKS_DEFAULT;
  params->lto_ms = LLCP_DEFAULT_LTO_MS;
  params->rw = LLCP_DEFAULT_RW;
}

void llcp_link_init(llcp_link_t *link) {
  if (link == NULL)
    return;
  memset(link, 0, sizeof(*link));
  llcp_params_default(&link->local);
  llcp_params_default(&link->remote);
  llcp_params_default(&link->negotiated);

  for (uint8_t i = 0; i < LLCP_NFCID3_LEN; i++) {
    uint32_t r = esp_random();
    link->nfcid3[i] = (uint8_t)(r & LLCP_BYTE_MASK);
  }

  link->did = 0;
  link->bs = 0;
  link->br = 0;
  link->pp = llcp_pp_from_miu(link->local.miu);
  link->lr = llcp_lr_from_pp(link->pp);
  link->pni = 0;
  link->llcp_active = false;
}

size_t llcp_tlv_version(uint8_t *out, size_t max, uint8_t version) {
  if (out == NULL || max < 3U)
    return 0;
  out[0] = LLCP_TLV_VERSION;
  out[1] = 1U;
  out[2] = version;
  return 3U;
}

size_t llcp_tlv_miux(uint8_t *out, size_t max, uint16_t miux) {
  if (out == NULL || max < 4U)
    return 0;
  uint16_t v = (uint16_t)(miux & LLCP_MIUX_MASK);
  out[0] = LLCP_TLV_MIUX;
  out[1] = 2U;
  out[2] = (uint8_t)((v >> 8) & LLCP_BYTE_MASK);
  out[3] = (uint8_t)(v & LLCP_BYTE_MASK);
  return 4U;
}

size_t llcp_tlv_wks(uint8_t *out, size_t max, uint16_t wks) {
  if (out == NULL || max < 4U)
    return 0;
  out[0] = LLCP_TLV_WKS;
  out[1] = 2U;
  out[2] = (uint8_t)((wks >> 8) & LLCP_BYTE_MASK);
  out[3] = (uint8_t)(wks & LLCP_BYTE_MASK);
  return 4U;
}

size_t llcp_tlv_lto(uint8_t *out, size_t max, uint8_t lto) {
  if (out == NULL || max < 3U)
    return 0;
  out[0] = LLCP_TLV_LTO;
  out[1] = 1U;
  out[2] = lto;
  return 3U;
}

size_t llcp_tlv_rw(uint8_t *out, size_t max, uint8_t rw) {
  if (out == NULL || max < 3U)
    return 0;
  out[0] = LLCP_TLV_RW;
  out[1] = 1U;
  out[2] = (uint8_t)(rw & LLCP_NIBBLE_MASK);
  return 3U;
}

size_t llcp_tlv_sn(uint8_t *out, size_t max, const char *sn) {
  if (out == NULL || sn == NULL)
    return 0;
  size_t len = strlen(sn);
  if (max < (2U + len))
    return 0;
  out[0] = LLCP_TLV_SN;
  out[1] = (uint8_t)len;
  memcpy(&out[2], sn, len);
  return 2U + len;
}

size_t llcp_build_gt(const llcp_params_t *params, uint8_t *out, size_t max) {
  if (params == NULL || out == NULL || max < 3U)
    return 0;

  size_t pos = 0;
  out[pos++] = LLCP_MAGIC_0;
  out[pos++] = LLCP_MAGIC_1;
  out[pos++] = LLCP_MAGIC_2;

  uint8_t ver =
      (uint8_t)((params->version_major << 4) | (params->version_minor & LLCP_NIBBLE_MASK));
  size_t n = llcp_tlv_version(&out[pos], max - pos, ver);
  if (n == 0)
    return 0;
  pos += n;

  n = llcp_tlv_wks(&out[pos], max - pos, params->wks);
  if (n == 0)
    return 0;
  pos += n;

  uint8_t lto = (uint8_t)((params->lto_ms + LLCP_LTO_UNIT_MS - 1U) / LLCP_LTO_UNIT_MS);
  n = llcp_tlv_lto(&out[pos], max - pos, lto);
  if (n == 0)
    return 0;
  pos += n;

  uint16_t miux =
      (params->miu > LLCP_DEFAULT_MIU) ? (uint16_t)(params->miu - LLCP_DEFAULT_MIU) : 0U;
  n = llcp_tlv_miux(&out[pos], max - pos, miux);
  if (n == 0)
    return 0;
  pos += n;

  n = llcp_tlv_rw(&out[pos], max - pos, params->rw);
  if (n == 0)
    return 0;
  pos += n;

  return pos;
}

bool llcp_parse_gt(const uint8_t *gt, size_t gt_len, llcp_params_t *out) {
  if (gt == NULL || gt_len < 3U || out == NULL)
    return false;

  llcp_params_default(out);
  if (gt[0] != LLCP_MAGIC_0 || gt[1] != LLCP_MAGIC_1 || gt[2] != LLCP_MAGIC_2)
    return false;

  size_t pos = LLCP_MAGIC_BYTES;
  while (pos + 2U <= gt_len) {
    uint8_t type = gt[pos++];
    uint8_t len = gt[pos++];
    if (pos + len > gt_len)
      break;

    switch (type) {
      case LLCP_TLV_VERSION:
        if (len >= 1) {
          out->version_major = (uint8_t)((gt[pos] >> 4) & LLCP_NIBBLE_MASK);
          out->version_minor = (uint8_t)(gt[pos] & LLCP_NIBBLE_MASK);
        }
        break;
      case LLCP_TLV_MIUX:
        if (len >= 2) {
          uint16_t miux = (uint16_t)((gt[pos] << 8) | gt[pos + 1]);
          miux &= LLCP_MIUX_MASK;
          out->miu = (uint16_t)(LLCP_DEFAULT_MIU + miux);
        }
        break;
      case LLCP_TLV_WKS:
        if (len >= 2) {
          out->wks = (uint16_t)((gt[pos] << 8) | gt[pos + 1]);
        }
        break;
      case LLCP_TLV_LTO:
        if (len >= 1) {
          out->lto_ms = (uint16_t)(gt[pos] * LLCP_LTO_UNIT_MS);
        }
        break;
      case LLCP_TLV_RW:
        if (len >= 1) {
          out->rw = (uint8_t)(gt[pos] & LLCP_NIBBLE_MASK);
        }
        break;
      default:
        break;
    }

    pos += len;
  }

  return true;
}

static void llcp_compute_negotiated(llcp_link_t *link) {
  if (link == NULL)
    return;

  uint8_t maj = link->local.version_major < link->remote.version_major ? link->local.version_major
                                                                       : link->remote.version_major;
  uint8_t min =
      (link->local.version_major == link->remote.version_major)
          ? (link->local.version_minor < link->remote.version_minor ? link->local.version_minor
                                                                    : link->remote.version_minor)
          : 0;

  link->negotiated.version_major = maj;
  link->negotiated.version_minor = min;
  link->negotiated.miu = (link->local.miu < link->remote.miu) ? link->local.miu : link->remote.miu;
  link->negotiated.wks = (uint16_t)(link->local.wks & link->remote.wks);
  link->negotiated.lto_ms =
      (link->local.lto_ms < link->remote.lto_ms) ? link->local.lto_ms : link->remote.lto_ms;
  link->negotiated.rw = (link->local.rw < link->remote.rw) ? link->local.rw : link->remote.rw;
}

hb_nfc_err_t llcp_initiator_activate(llcp_link_t *link) {
  if (link == NULL)
    return HB_NFC_ERR_PARAM;

  nfc_rf_config_t cfg = {
      .tech = NFC_RF_TECH_A,
      .tx_rate = NFC_RF_BR_106,
      .rx_rate = NFC_RF_BR_106,
      .am_mod_percent = 0,
      .tx_parity = true,
      .rx_raw_parity = false,
      .guard_time_us = 0,
      .fdt_min_us = 0,
      .validate_fdt = false,
  };
  hb_nfc_err_t err = nfc_rf_apply(&cfg);
  if (err != HB_NFC_OK)
    return err;

  if (!st25r3916_core_field_is_on()) {
    err = st25r3916_core_field_on();
    if (err != HB_NFC_OK)
      return err;
  }

  uint8_t gt[64];
  size_t gt_len = llcp_build_gt(&link->local, gt, sizeof(gt));
  if (gt_len == 0)
    return HB_NFC_ERR_INTERNAL;

  uint8_t atr[96];
  size_t pos = 0;
  atr[pos++] = NFCDEP_CMD0_REQ;
  atr[pos++] = NFCDEP_CMD1_ATR_REQ;
  memcpy(&atr[pos], link->nfcid3, LLCP_NFCID3_LEN);
  pos += LLCP_NFCID3_LEN;
  atr[pos++] = link->did;
  atr[pos++] = link->bs;
  atr[pos++] = link->br;
  link->pp = llcp_pp_from_miu(link->local.miu);
  atr[pos++] = link->pp;
  memcpy(&atr[pos], gt, gt_len);
  pos += gt_len;

  uint8_t rx[128] = {0};
  int len = nfc_poller_transceive(atr, pos, true, rx, sizeof(rx), 1, LLCP_DEP_DEFAULT_TMO);
  if (len < LLCP_ATR_RES_MIN_LEN)
    return HB_NFC_ERR_NO_CARD;
  len = llcp_strip_crc(rx, len);

  if (rx[0] != NFCDEP_CMD0_RES || rx[1] != NFCDEP_CMD1_ATR_RES) {
    ESP_LOGW(TAG, "ATR_RES invalid: %02X %02X", rx[0], rx[1]);
    return HB_NFC_ERR_PROTOCOL;
  }

  link->did = rx[LLCP_ATR_RES_DID_OFF];
  link->bs = rx[LLCP_ATR_RES_BS_OFF];
  link->br = rx[LLCP_ATR_RES_BR_OFF];
  link->to = rx[LLCP_ATR_RES_TO_OFF];
  link->pp = rx[LLCP_ATR_RES_PP_OFF];
  link->lr = llcp_lr_from_pp(link->pp);
  link->rwt_ms = llcp_rwt_ms(link->to);

  size_t gt_off = LLCP_ATR_RES_MIN_LEN;
  if (gt_off < (size_t)len) {
    if (!llcp_parse_gt(&rx[gt_off], (size_t)len - gt_off, &link->remote)) {
      ESP_LOGW(TAG, "LLCP magic not found");
      return HB_NFC_ERR_PROTOCOL;
    }
  } else {
    return HB_NFC_ERR_PROTOCOL;
  }

  llcp_compute_negotiated(link);
  link->pni = 0;
  link->llcp_active = true;
  ESP_LOGI(TAG,
           "LLCP active: MIU=%u RW=%u LTO=%ums",
           (unsigned)link->negotiated.miu,
           (unsigned)link->negotiated.rw,
           (unsigned)link->negotiated.lto_ms);
  return HB_NFC_OK;
}

static int nfc_dep_transceive(llcp_link_t *link,
                              const uint8_t *tx,
                              size_t tx_len,
                              uint8_t *rx,
                              size_t rx_max,
                              int timeout_ms) {
  if (link == NULL || tx == NULL || tx_len == 0 || rx == NULL || rx_max == 0)
    return 0;

  uint16_t lr = link->lr ? link->lr : LLCP_LR_64;
  if (tx_len > lr)
    return 0;

  uint8_t frame[NFCDEP_HDR_SIZE + NFCDEP_MAX_PAYLOAD];
  size_t pos = 0;
  frame[pos++] = NFCDEP_CMD0_REQ;
  frame[pos++] = NFCDEP_CMD1_DEP_REQ;
  frame[pos++] = (uint8_t)(link->pni & NFCDEP_PFB_PNI_MASK);
  memcpy(&frame[pos], tx, tx_len);
  pos += tx_len;

  uint8_t rbuf[512] = {0};
  int tmo =
      (timeout_ms > 0) ? timeout_ms : (int)(link->rwt_ms ? link->rwt_ms : LLCP_DEP_DEFAULT_TMO);
  int rlen = nfc_poller_transceive(frame, pos, true, rbuf, sizeof(rbuf), 1, tmo);
  if (rlen < NFCDEP_HDR_SIZE)
    return 0;
  rlen = llcp_strip_crc(rbuf, rlen);

  if (rbuf[0] != NFCDEP_CMD0_RES || rbuf[1] != NFCDEP_CMD1_DEP_RES) {
    ESP_LOGW(TAG, "DEP_RES invalid: %02X %02X", rbuf[0], rbuf[1]);
    return 0;
  }

  uint8_t pfb = rbuf[2];
  size_t off = NFCDEP_HDR_SIZE;
  if (pfb & NFCDEP_PFB_DID)
    off++;
  if (pfb & NFCDEP_PFB_NAD)
    off++;
  if (pfb & NFCDEP_PFB_MI) {
    ESP_LOGW(TAG, "DEP chaining not supported");
    return 0;
  }

  if (off >= (size_t)rlen)
    return 0;
  size_t payload_len = (size_t)rlen - off;
  if (payload_len > rx_max)
    payload_len = rx_max;
  memcpy(rx, &rbuf[off], payload_len);

  link->pni = (uint8_t)((link->pni + 1U) & LLCP_PNI_MASK);
  return (int)payload_len;
}

int llcp_exchange_pdu(llcp_link_t *link,
                      const uint8_t *tx_pdu,
                      size_t tx_len,
                      uint8_t *rx_pdu,
                      size_t rx_max,
                      int timeout_ms) {
  if (link == NULL || !link->llcp_active)
    return 0;
  return nfc_dep_transceive(link, tx_pdu, tx_len, rx_pdu, rx_max, timeout_ms);
}

size_t llcp_build_header(uint8_t dsap, uint8_t ptype, uint8_t ssap, uint8_t *out, size_t max) {
  if (out == NULL || max < 2U)
    return 0;
  out[0] = (uint8_t)((dsap << 2) | ((ptype >> 2) & LLCP_PTYPE_LO_MASK));
  out[1] = (uint8_t)(((ptype & LLCP_PTYPE_LO_MASK) << 6) | (ssap & LLCP_SAP_MASK));
  return 2U;
}

bool llcp_parse_header(const uint8_t *pdu,
                       size_t len,
                       uint8_t *dsap,
                       uint8_t *ptype,
                       uint8_t *ssap,
                       uint8_t *ns,
                       uint8_t *nr) {
  if (pdu == NULL || len < 2U)
    return false;
  uint8_t d = (uint8_t)(pdu[0] >> 2);
  uint8_t p = (uint8_t)(((pdu[0] & LLCP_PTYPE_LO_MASK) << 2) | (pdu[1] >> 6));
  uint8_t s = (uint8_t)(pdu[1] & LLCP_SAP_MASK);

  if (dsap != NULL)
    *dsap = d;
  if (ptype != NULL)
    *ptype = p;
  if (ssap != NULL)
    *ssap = s;

  if ((p == LLCP_PTYPE_I || p == LLCP_PTYPE_RR || p == LLCP_PTYPE_RNR) && len >= 3U) {
    if (ns != NULL)
      *ns = (uint8_t)(pdu[2] >> 4);
    if (nr != NULL)
      *nr = (uint8_t)(pdu[2] & LLCP_NIBBLE_MASK);
  }
  return true;
}

size_t llcp_build_symm(uint8_t *out, size_t max) {
  return llcp_build_header(0, LLCP_PTYPE_SYMM, 0, out, max);
}

size_t llcp_build_ui(
    uint8_t dsap, uint8_t ssap, const uint8_t *info, size_t info_len, uint8_t *out, size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_UI, ssap, out, max);
  if (pos == 0)
    return 0;
  if (info == NULL || info_len == 0)
    return pos;
  if (pos + info_len > max)
    return 0;
  memcpy(&out[pos], info, info_len);
  return pos + info_len;
}

size_t llcp_build_connect(uint8_t dsap,
                          uint8_t ssap,
                          const char *service_name,
                          const llcp_params_t *params,
                          uint8_t *out,
                          size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_CONNECT, ssap, out, max);
  if (pos == 0)
    return 0;

  llcp_params_t p;
  if (params != NULL)
    p = *params;
  else
    llcp_params_default(&p);

  size_t n = llcp_tlv_rw(&out[pos], max - pos, p.rw);
  if (n == 0)
    return 0;
  pos += n;

  uint16_t miux = (p.miu > LLCP_DEFAULT_MIU) ? (uint16_t)(p.miu - LLCP_DEFAULT_MIU) : 0U;
  n = llcp_tlv_miux(&out[pos], max - pos, miux);
  if (n == 0)
    return 0;
  pos += n;

  if (service_name != NULL) {
    n = llcp_tlv_sn(&out[pos], max - pos, service_name);
    if (n == 0)
      return 0;
    pos += n;
  }

  return pos;
}

size_t
llcp_build_cc(uint8_t dsap, uint8_t ssap, const llcp_params_t *params, uint8_t *out, size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_CC, ssap, out, max);
  if (pos == 0)
    return 0;

  llcp_params_t p;
  if (params != NULL)
    p = *params;
  else
    llcp_params_default(&p);

  size_t n = llcp_tlv_rw(&out[pos], max - pos, p.rw);
  if (n == 0)
    return 0;
  pos += n;

  uint16_t miux = (p.miu > LLCP_DEFAULT_MIU) ? (uint16_t)(p.miu - LLCP_DEFAULT_MIU) : 0U;
  n = llcp_tlv_miux(&out[pos], max - pos, miux);
  if (n == 0)
    return 0;
  pos += n;

  return pos;
}

size_t llcp_build_disc(uint8_t dsap, uint8_t ssap, uint8_t *out, size_t max) {
  return llcp_build_header(dsap, LLCP_PTYPE_DISC, ssap, out, max);
}

size_t llcp_build_dm(uint8_t dsap, uint8_t ssap, uint8_t reason, uint8_t *out, size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_DM, ssap, out, max);
  if (pos == 0 || max < pos + 1U)
    return 0;
  out[pos++] = reason;
  return pos;
}

size_t llcp_build_i(uint8_t dsap,
                    uint8_t ssap,
                    uint8_t ns,
                    uint8_t nr,
                    const uint8_t *info,
                    size_t info_len,
                    uint8_t *out,
                    size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_I, ssap, out, max);
  if (pos == 0 || max < pos + 1U)
    return 0;
  out[pos++] = (uint8_t)(((ns & LLCP_NIBBLE_MASK) << 4) | (nr & LLCP_NIBBLE_MASK));
  if (info != NULL && info_len > 0) {
    if (pos + info_len > max)
      return 0;
    memcpy(&out[pos], info, info_len);
    pos += info_len;
  }
  return pos;
}

size_t llcp_build_rr(uint8_t dsap, uint8_t ssap, uint8_t nr, uint8_t *out, size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_RR, ssap, out, max);
  if (pos == 0 || max < pos + 1U)
    return 0;
  out[pos++] = (uint8_t)(nr & LLCP_NIBBLE_MASK);
  return pos;
}

size_t llcp_build_rnr(uint8_t dsap, uint8_t ssap, uint8_t nr, uint8_t *out, size_t max) {
  size_t pos = llcp_build_header(dsap, LLCP_PTYPE_RNR, ssap, out, max);
  if (pos == 0 || max < pos + 1U)
    return 0;
  out[pos++] = (uint8_t)(nr & LLCP_NIBBLE_MASK);
  return pos;
}
