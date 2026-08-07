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
 * @file pin_def.h
 * @brief Central GPIO pin assignments for the Pancake board (ESP32-C5).
 *
 * Hardware: ESP32-C5-DevKitC-1 + ST7796S 320x480 (portrait) + FT6336
 * capacitive touch + micro-SD (shared SPI) + WS2812 LED + passive piezo.
 *
 * Pinout authoritative source: Ghost_ESP configs/sdkconfig.Pancake, plus
 * user-specified passive buzzer on GPIO6.
 *
 * Macro names from the ESP32-P4 target are preserved so absent-hardware
 * drivers (CC1101, SX1262, YS-RFID2, P4<->C5 bridge, SDMMC) still compile;
 * those peripherals do not exist on Pancake and their drivers are stubbed.
 */

#ifndef PIN_DEF_H
#define PIN_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

// -------- SPI Bus (SPI2_HOST) — shared: display + SD card ------------------
#define GPIO_SPI_MOSI_PIN 24
#define GPIO_SPI_SCLK_PIN 23
#define GPIO_SPI_MISO_PIN 4

// -------- ST7796S Display (portrait 320x480) -------------------------------
// Original ST7789 macro names are repurposed for the ST7796S panel so any
// existing references (e.g. backlight control) keep working.
#define GPIO_ST7789_CS_PIN  5
#define GPIO_ST7789_DC_PIN  3   // strapping pin — gpio_reset_pin() before use
#define GPIO_ST7789_RST_PIN 2   // strapping pin — gpio_reset_pin() before use
#define GPIO_ST7789_BL_PIN  26  // backlight, active-high

// Explicit ST7796 aliases (preferred in new pancake code)
#define GPIO_ST7796_CS_PIN  GPIO_ST7789_CS_PIN
#define GPIO_ST7796_DC_PIN  GPIO_ST7789_DC_PIN
#define GPIO_ST7796_RST_PIN GPIO_ST7789_RST_PIN
#define GPIO_ST7796_BL_PIN  GPIO_ST7789_BL_PIN

// -------- FT6336 Capacitive Touch (I2C) ------------------------------------
#define GPIO_TOUCH_SDA_PIN 9
#define GPIO_TOUCH_SCL_PIN 10
#define GPIO_TOUCH_RST_PIN 8    // active-low
#define GPIO_TOUCH_INT_PIN 25

// Shared I2C bus mirrors the touch bus on Pancake.
#define GPIO_I2C_SDA_PIN GPIO_TOUCH_SDA_PIN
#define GPIO_I2C_SCL_PIN GPIO_TOUCH_SCL_PIN

// -------- micro-SD (SPI, shared bus) ---------------------------------------
#define GPIO_SD_CS_PIN 7

// -------- Passive piezo buzzer (LEDC PWM tone) -----------------------------
#define GPIO_BUZZER_PIN 6

// -------- RGB LED (WS2812) -------------------------------------------------
#define GPIO_LED_RGB_PIN 27
#define LED_COUNT        1

// ===========================================================================
// Absent on Pancake — placeholder pins kept only so stubbed drivers compile.
// These peripherals are NOT present; their init is never called.
// ===========================================================================

// CC1101 Sub-GHz Radio (absent)
#define GPIO_CC1101_CS_PIN   -1
#define GPIO_CC1101_GDO0_PIN -1
#define GPIO_CC1101_GDO2_PIN -1

// SDMMC 4-bit (absent — Pancake uses SPI SD above)
#define GPIO_SDMMC_CLK_PIN -1
#define GPIO_SDMMC_CMD_PIN -1
#define GPIO_SDMMC_D0_PIN  -1
#define GPIO_SDMMC_D1_PIN  -1
#define GPIO_SDMMC_D2_PIN  -1
#define GPIO_SDMMC_D3_PIN  -1

// Buttons (absent — Pancake is touch-only; kept for source compatibility)
#define GPIO_BTN_LEFT_PIN  -1
#define GPIO_BTN_BACK_PIN  -1
#define GPIO_BTN_UP_PIN    -1
#define GPIO_BTN_DOWN_PIN  -1
#define GPIO_BTN_OK_PIN    -1
#define GPIO_BTN_RIGHT_PIN -1

// P4-C5 Bridge SPI (absent — single-chip build)
#define GPIO_BRIDGE_SCLK_PIN -1
#define GPIO_BRIDGE_MOSI_PIN -1
#define GPIO_BRIDGE_MISO_PIN -1
#define GPIO_BRIDGE_CS_PIN   -1
#define GPIO_BRIDGE_IRQ_PIN  -1

// C5 Control & Update (absent — single-chip build)
#define GPIO_C5_UART_TX_PIN -1
#define GPIO_C5_UART_RX_PIN -1
#define GPIO_C5_RESET_PIN   -1
#define GPIO_C5_BOOT_PIN    -1

// SX1262 LoRa (absent)
#define GPIO_LORA_SCLK_PIN  -1
#define GPIO_LORA_MOSI_PIN  -1
#define GPIO_LORA_MISO_PIN  -1
#define GPIO_LORA_CS_PIN    -1
#define GPIO_LORA_RESET_PIN -1
#define GPIO_LORA_BUSY_PIN  -1
#define GPIO_LORA_DIO1_PIN  -1
#define GPIO_LORA_TXEN_PIN  -1
#define GPIO_LORA_RXEN_PIN  -1

// YS-RFID2 125kHz RFID Reader (absent)
#define GPIO_RFID_UART_TX_PIN -1
#define GPIO_RFID_UART_RX_PIN -1

#ifdef __cplusplus
}
#endif

#endif // PIN_DEF_H
