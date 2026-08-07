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

#ifndef HB_NFC_GPIO_H
#define HB_NFC_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

/**
 * @brief Configure the ST25R3916 IRQ pin as a digital input.
 *
 * @param pin_irq  GPIO pin connected to the IRQ output of the ST25R3916 (active high).
 * @return
 *   - ESP_OK on success
 *   - ESP_FAIL if gpio_config() fails
 */
esp_err_t hb_nfc_gpio_init(gpio_num_t pin_irq);

/**
 * @brief Release and reset the IRQ GPIO pin.
 */
void hb_nfc_gpio_deinit(void);

/**
 * @brief Sample the IRQ pin level.
 *
 * @return true if the IRQ pin is high, false if low or if not initialized.
 */
bool hb_nfc_gpio_irq_level(void);

/**
 * @brief Poll the IRQ pin high with a millisecond timeout.
 *
 * @param timeout_ms  Maximum wait time in milliseconds.
 * @return true if the pin went high within the timeout, false on timeout.
 */
bool hb_nfc_gpio_irq_wait(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // HB_NFC_GPIO_H
