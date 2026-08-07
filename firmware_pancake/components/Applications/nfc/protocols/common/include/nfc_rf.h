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
 * @file nfc_rf.h
 * @brief RF layer configuration (Phase 2): bitrate, modulation, parity, timing.
 */
#ifndef NFC_RF_H
#define NFC_RF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "highboy_nfc_error.h"
#include "highboy_nfc_compat.h"

/**
 * @brief NFC RF technology types.
 */
typedef enum {
  NFC_RF_TECH_A = 0,
  NFC_RF_TECH_B,
  NFC_RF_TECH_F,
  NFC_RF_TECH_V,
  NFC_RF_TECH_COUNT
} nfc_rf_tech_t;

/**
 * @brief NFC RF bitrate options.
 */
typedef enum {
  NFC_RF_BR_106 = 0,
  NFC_RF_BR_212,
  NFC_RF_BR_424,
  NFC_RF_BR_848,
  NFC_RF_BR_V_LOW,  /**< NFC-V low data rate (6.62 kbps RX / 1.65 kbps TX) */
  NFC_RF_BR_V_HIGH, /**< NFC-V high data rate (26.48 kbps) */
  NFC_RF_BR_COUNT
} nfc_rf_bitrate_t;

/**
 * @brief RF layer configuration parameters.
 */
typedef struct {
  nfc_rf_tech_t tech;
  nfc_rf_bitrate_t tx_rate;
  nfc_rf_bitrate_t rx_rate;
  uint8_t am_mod_percent; /* 0 = keep current */
  bool tx_parity;         /* ISO14443A: generate parity */
  bool rx_raw_parity;     /* ISO14443A: receive parity bits in FIFO (no check) */
  uint32_t guard_time_us; /* optional delay before TX */
  uint32_t fdt_min_us;    /* optional minimum FDT */
  bool validate_fdt;      /* log warning if FDT < min */
} nfc_rf_config_t;

/**
 * @brief Apply RF configuration for a given technology.
 *
 * @param cfg  Pointer to the RF configuration structure.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if cfg is NULL
 *   - HB_NFC_ERR on hardware failure
 */
hb_nfc_err_t nfc_rf_apply(const nfc_rf_config_t *cfg);

/**
 * @brief Set the TX and RX bitrate for NFC-A, B, or F.
 *
 * @param tx  Transmit bitrate.
 * @param rx  Receive bitrate.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if the bitrate is unsupported
 */
hb_nfc_err_t nfc_rf_set_bitrate(nfc_rf_bitrate_t tx, nfc_rf_bitrate_t rx);

/**
 * @brief Set the raw BIT_RATE register value directly.
 *
 * @param value  Raw register value to write.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR on hardware failure
 */
hb_nfc_err_t nfc_rf_set_bitrate_raw(uint8_t value);

/**
 * @brief Set the AM modulation index in percent.
 *
 * @param percent  Modulation index (valid range: 5..40).
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR_INVALID_ARG if percent is out of range
 */
hb_nfc_err_t nfc_rf_set_am_modulation(uint8_t percent);

/**
 * @brief Configure ISO14443A TX parity generation and RX parity handling.
 *
 * @param tx_parity      Enable TX parity bit generation.
 * @param rx_raw_parity  Receive parity bits in FIFO without checking.
 * @return
 *   - HB_NFC_OK on success
 *   - HB_NFC_ERR on hardware failure
 */
hb_nfc_err_t nfc_rf_set_parity(bool tx_parity, bool rx_raw_parity);

/**
 * @brief Configure guard time and frame delay time validation.
 *
 * @param guard_time_us  Guard time before TX in microseconds.
 * @param fdt_min_us     Minimum frame delay time in microseconds.
 * @param validate_fdt   Log a warning if the measured FDT is below the minimum.
 */
void nfc_rf_set_timing(uint32_t guard_time_us, uint32_t fdt_min_us, bool validate_fdt);

/**
 * @brief Get the last measured frame delay time.
 *
 * @return Last measured FDT in microseconds.
 */
uint32_t nfc_rf_get_last_fdt_us(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_RF_H */
