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

#ifndef HB_NFC_TIMER_H
#define HB_NFC_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Busy-wait delay in microseconds.
 *
 * Uses the ESP-ROM implementation. Safe for ISRs, but blocks the CPU.
 *
 * @param us Delay in microseconds.
 */
void hb_nfc_timer_delay_us(uint32_t us);

/**
 * @brief OS-aware delay in milliseconds.
 *
 * Yields the current task to the FreeRTOS scheduler.
 * Must NOT be called from an ISR.
 *
 * @param ms Delay in milliseconds. If 0, performs a task yield.
 */
void hb_nfc_timer_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // HB_NFC_TIMER_H
