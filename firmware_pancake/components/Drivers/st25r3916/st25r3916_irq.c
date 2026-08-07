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

#include "st25r3916_irq.h"

#include <string.h>

#include "esp_log.h"

#include "st25r3916_reg.h"
#include "hb_nfc_spi.h"
#include "hb_nfc_gpio.h"
#include "hb_nfc_timer.h"

static const char *TAG = "ST25R3916_IRQ";

esp_err_t st25r3916_irq_read(st25r3916_irq_status_t *out_status) {
  if (out_status == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  hb_nfc_spi_reg_read(ST25R3916_REG_ERROR_INT, &out_status->error);
  hb_nfc_spi_reg_read(ST25R3916_REG_TIMER_NFC_INT, &out_status->timer);
  hb_nfc_spi_reg_read(ST25R3916_REG_MAIN_INT, &out_status->main);
  hb_nfc_spi_reg_read(ST25R3916_REG_TARGET_INT, &out_status->target);
  hb_nfc_spi_reg_read(ST25R3916_REG_COLLISION, &out_status->collision);

  return ESP_OK;
}

void st25r3916_irq_log(const char *ctx, uint16_t fifo_count) {
  if (ctx == NULL) {
    return;
  }

  st25r3916_irq_status_t status = {0};
  if (st25r3916_irq_read(&status) == ESP_OK) {
    ESP_LOGW(TAG,
             "%s IRQ: MAIN=0x%02X ERR=0x%02X TMR=0x%02X TGT=0x%02X COL=0x%02X FIFO=%u",
             ctx,
             status.main,
             status.error,
             status.timer,
             status.target,
             status.collision,
             fifo_count);
  }
}

esp_err_t st25r3916_irq_wait_txe(void) {
  for (int i = 0; i < ST25R3916_IRQ_TX_POLL_MAX; i++) {
    if (hb_nfc_gpio_irq_level()) {
      st25r3916_irq_status_t status = {0};
      if (st25r3916_irq_read(&status) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read IRQ status");
        return ESP_FAIL;
      }

      if (status.main & ST25R3916_IRQ_MAIN_TXE) {
        return ESP_OK;
      }

      if (status.error != 0) {
        ESP_LOGW(TAG, "TX error: ERR=0x%02X", status.error);
        return ESP_FAIL;
      }
    }
    hb_nfc_timer_delay_us(ST25R3916_IRQ_TX_POLL_US);
  }

  ESP_LOGW(TAG, "TX timeout");
  return ESP_ERR_TIMEOUT;
}
