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

#include "ft6336.h"

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "pin_def.h"
#include "st7789.h"  // LCD_H_RES / LCD_V_RES

static const char *TAG = "FT6336";

#define FT6336_ADDR       0x38
#define FT6336_I2C_PORT   I2C_NUM_0
#define FT6336_I2C_TMO_MS 50

// Registers
#define FT6336_REG_TD_STATUS 0x02  // number of touch points (low nibble)
#define FT6336_REG_P1_XH     0x03  // event flag (bits 7:6) + X high nibble
#define FT6336_REG_P1_XL     0x04
#define FT6336_REG_P1_YH     0x05
#define FT6336_REG_P1_YL     0x06
#define FT6336_REG_VENDOR_ID 0xA8

// Touch event flags (bits 7:6 of P1_XH)
#define FT6336_EVENT_LIFT_UP 0x01

// Native touch panel dimensions (portrait, matches ST7796S 320x480).
#define TOUCH_NATIVE_W 320
#define TOUCH_NATIVE_H 480

// Calibration knobs — adjust on-device if taps land in the wrong zone.
// Defaults assume the ST7796S is driven at rotation 1 (0deg); the
// M5PORKCHOP reference flips both axes because it drives the panel at
// rotation 2 (180deg), so with rotation 1 no flip is expected.
#define TOUCH_SWAP_XY  0
#define TOUCH_INVERT_X 0
#define TOUCH_INVERT_Y 0

static esp_err_t ft6336_read_reg(uint8_t reg, uint8_t *val) {
  return i2c_master_write_read_device(FT6336_I2C_PORT, FT6336_ADDR, &reg, 1, val, 1,
                                      pdMS_TO_TICKS(FT6336_I2C_TMO_MS));
}

esp_err_t ft6336_init(void) {
  if (GPIO_TOUCH_RST_PIN >= 0) {
    gpio_reset_pin(GPIO_TOUCH_RST_PIN);
    gpio_set_direction(GPIO_TOUCH_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_TOUCH_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(GPIO_TOUCH_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  uint8_t vendor = 0;
  esp_err_t ret = ft6336_read_reg(FT6336_REG_VENDOR_ID, &vendor);
  if (ret != ESP_OK || vendor == 0x00 || vendor == 0xFF) {
    ESP_LOGE(TAG, "FT6336 not detected (ret=%s, vendor=0x%02X)", esp_err_to_name(ret), vendor);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "FT6336 detected (vendor 0x%02X) on SDA=%d SCL=%d", vendor, GPIO_I2C_SDA_PIN,
           GPIO_I2C_SCL_PIN);
  return ESP_OK;
}

bool ft6336_get_point(int16_t *x, int16_t *y) {
  uint8_t td = 0;
  if (ft6336_read_reg(FT6336_REG_TD_STATUS, &td) != ESP_OK) {
    return false;
  }
  uint8_t points = td & 0x0F;
  if (points == 0 || points > 5) {
    return false;
  }

  uint8_t xh, xl, yh, yl;
  if (ft6336_read_reg(FT6336_REG_P1_XH, &xh) != ESP_OK ||
      ft6336_read_reg(FT6336_REG_P1_XL, &xl) != ESP_OK ||
      ft6336_read_reg(FT6336_REG_P1_YH, &yh) != ESP_OK ||
      ft6336_read_reg(FT6336_REG_P1_YL, &yl) != ESP_OK) {
    return false;
  }

  uint8_t event = (xh >> 6) & 0x03;
  if (event == FT6336_EVENT_LIFT_UP) {
    return false;
  }

  int16_t rx = (int16_t)(((uint16_t)(xh & 0x0F) << 8) | xl);
  int16_t ry = (int16_t)(((uint16_t)(yh & 0x0F) << 8) | yl);

#if TOUCH_INVERT_X
  rx = (TOUCH_NATIVE_W - 1) - rx;
#endif
#if TOUCH_INVERT_Y
  ry = (TOUCH_NATIVE_H - 1) - ry;
#endif
#if TOUCH_SWAP_XY
  int16_t tmp = rx;
  rx = ry;
  ry = tmp;
#endif

  // Clamp into display space.
  if (rx < 0) rx = 0;
  if (ry < 0) ry = 0;
  if (rx >= LCD_H_RES) rx = LCD_H_RES - 1;
  if (ry >= LCD_V_RES) ry = LCD_V_RES - 1;

  *x = rx;
  *y = ry;
  return true;
}
