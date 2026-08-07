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

#ifndef SX1262_IRQ_H
#define SX1262_IRQ_H

#include "esp_err.h"
#include "sx1262_types.h"
#include "sx1262_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Ring Buffer Config ───────────────────────────────────────────── */
#define SX1262_RX_RING_SIZE 8

/**
 * @brief Initialize IRQ subsystem. Resets ring buffer.
 */
void sx1262_irq_init(sx1262_hal_t *hal,
                     const sx1262_config_t *config,
                     const sx1262_callbacks_t *cbs);

/**
 * @brief Process pending IRQ flags.
 *
 * Called by ISR task when DIO1 rises. Never from ISR directly.
 * Sequence: GetIrqStatus → ClearIrqStatus → process → callbacks → FSM update.
 *
 * @return ESP_OK on success.
 */
esp_err_t sx1262_irq_process(void);

/**
 * @brief Dequeue next received packet from ring buffer.
 *
 * Thread-safe via enter_critical/exit_critical.
 *
 * @param out_packet  Destination for packet data. Must not be NULL.
 * @return ESP_OK if packet available, ESP_ERR_NOT_FOUND if empty.
 */
esp_err_t sx1262_irq_get_packet(sx1262_packet_t *out_packet);

/**
 * @brief Check if ring buffer has packets available.
 */
bool sx1262_irq_has_packet(void);

#ifdef __cplusplus
}
#endif

#endif // SX1262_IRQ_H
