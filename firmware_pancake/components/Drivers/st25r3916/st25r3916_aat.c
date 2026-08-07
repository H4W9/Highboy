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

#include "st25r3916_aat.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

#include "st25r3916_core.h"
#include "st25r3916_cmd.h"
#include "st25r3916_reg.h"
#include "hb_nfc_spi.h"
#include "hb_nfc_timer.h"

static const char *TAG = "ST25R3916_AAT";

#define ST25R3916_AAT_DAC_DEFAULT 0x80
#define ST25R3916_AAT_DAC_MAX     0xFF
#define ST25R3916_AAT_COARSE_STEP 16
#define ST25R3916_AAT_FINE_STEP   4

static esp_err_t set_dacs(uint8_t dac_a, uint8_t dac_b) {
  if (hb_nfc_spi_reg_write(ST25R3916_REG_ANT_TUNE_A, dac_a) != ESP_OK)
    return ESP_FAIL;
  if (hb_nfc_spi_reg_write(ST25R3916_REG_ANT_TUNE_B, dac_b) != ESP_OK)
    return ESP_FAIL;
  return ESP_OK;
}

static void perform_measurement(uint8_t *out_amp, uint8_t *out_phase) {
  hb_nfc_spi_direct_cmd(ST25R3916_CMD_MEAS_AMPLITUDE);
  hb_nfc_timer_delay_ms(ST25R3916_AD_MEAS_DELAY_MS);
  hb_nfc_spi_reg_read(ST25R3916_REG_AD_RESULT, out_amp);

  hb_nfc_spi_direct_cmd(ST25R3916_CMD_MEAS_PHASE);
  hb_nfc_timer_delay_ms(ST25R3916_AD_MEAS_DELAY_MS);
  hb_nfc_spi_reg_read(ST25R3916_REG_AD_RESULT, out_phase);
}

esp_err_t st25r3916_aat_calibrate(st25r3916_aat_result_t *out_result) {
  if (out_result == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  bool original_field_state = st25r3916_core_field_is_on();
  if (!original_field_state) {
    if (st25r3916_core_field_on() != ESP_OK) {
      return ESP_FAIL;
    }
  }

  uint8_t best_a = ST25R3916_AAT_DAC_DEFAULT;
  uint8_t best_b = ST25R3916_AAT_DAC_DEFAULT;
  uint8_t max_amp = 0;
  uint8_t best_ph = 0;

  for (uint16_t a = 0; a <= ST25R3916_AAT_DAC_MAX; a += ST25R3916_AAT_COARSE_STEP) {
    for (uint16_t b = 0; b <= ST25R3916_AAT_DAC_MAX; b += ST25R3916_AAT_COARSE_STEP) {
      set_dacs((uint8_t)a, (uint8_t)b);
      uint8_t amp = 0, ph = 0;
      perform_measurement(&amp, &ph);
      if (amp > max_amp) {
        max_amp = amp;
        best_ph = ph;
        best_a = (uint8_t)a;
        best_b = (uint8_t)b;
      }
    }
  }

  int start_a = (best_a > ST25R3916_AAT_COARSE_STEP) ? (best_a - ST25R3916_AAT_COARSE_STEP) : 0;
  int end_a = (best_a + ST25R3916_AAT_COARSE_STEP > ST25R3916_AAT_DAC_MAX)
                  ? ST25R3916_AAT_DAC_MAX
                  : (best_a + ST25R3916_AAT_COARSE_STEP);
  int start_b = (best_b > ST25R3916_AAT_COARSE_STEP) ? (best_b - ST25R3916_AAT_COARSE_STEP) : 0;
  int end_b = (best_b + ST25R3916_AAT_COARSE_STEP > ST25R3916_AAT_DAC_MAX)
                  ? ST25R3916_AAT_DAC_MAX
                  : (best_b + ST25R3916_AAT_COARSE_STEP);

  for (int a = start_a; a <= end_a; a += ST25R3916_AAT_FINE_STEP) {
    for (int b = start_b; b <= end_b; b += ST25R3916_AAT_FINE_STEP) {
      set_dacs((uint8_t)a, (uint8_t)b);
      uint8_t amp = 0, ph = 0;
      perform_measurement(&amp, &ph);
      if (amp > max_amp) {
        max_amp = amp;
        best_ph = ph;
        best_a = (uint8_t)a;
        best_b = (uint8_t)b;
      }
    }
  }

  set_dacs(best_a, best_b);
  out_result->dac_a = best_a;
  out_result->dac_b = best_b;
  out_result->amplitude = max_amp;
  out_result->phase = best_ph;

  if (!original_field_state) {
    st25r3916_core_field_off();
  }

  ESP_LOGI(TAG, "Calibration finished: A=0x%02X B=0x%02X Amp=%u", best_a, best_b, max_amp);
  return ESP_OK;
}

esp_err_t st25r3916_aat_load_nvs(st25r3916_aat_result_t *out_result) {
  if (out_result == NULL)
    return ESP_ERR_INVALID_ARG;

  nvs_handle_t handle;
  esp_err_t err = nvs_open("hb_aat", NVS_READONLY, &handle);
  if (err != ESP_OK)
    return err;

  err = nvs_get_u8(handle, "dac_a", &out_result->dac_a);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }
  err = nvs_get_u8(handle, "dac_b", &out_result->dac_b);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }
  err = nvs_get_u8(handle, "amp", &out_result->amplitude);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }
  err = nvs_get_u8(handle, "phase", &out_result->phase);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }

  nvs_close(handle);
  return ESP_OK;
}

esp_err_t st25r3916_aat_save_nvs(const st25r3916_aat_result_t *result) {
  if (result == NULL)
    return ESP_ERR_INVALID_ARG;

  nvs_handle_t handle;
  esp_err_t err = nvs_open("hb_aat", NVS_READWRITE, &handle);
  if (err != ESP_OK)
    return err;

  err = nvs_set_u8(handle, "dac_a", result->dac_a);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }
  err = nvs_set_u8(handle, "dac_b", result->dac_b);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }
  err = nvs_set_u8(handle, "amp", result->amplitude);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }
  err = nvs_set_u8(handle, "phase", result->phase);
  if (err != ESP_OK) {
    nvs_close(handle);
    return err;
  }

  err = nvs_commit(handle);
  nvs_close(handle);
  return err;
}
