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

#ifndef SX1262_FSM_H
#define SX1262_FSM_H

#include "esp_err.h"
#include "sx1262_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize FSM to a known state.
 */
void sx1262_fsm_init(sx1262_state_t initial_state);

/**
 * @brief Get current FSM state.
 */
sx1262_state_t sx1262_fsm_get_state(void);

/**
 * @brief Validate and execute a state transition.
 *
 * Based strictly on DS Fig. 9-1 transition diagram.
 * Returns ESP_ERR_INVALID_STATE if the transition is not allowed.
 *
 * @param target  Desired target state.
 * @return ESP_OK if transition is valid and state was updated.
 */
esp_err_t sx1262_fsm_transition(sx1262_state_t target);

/**
 * @brief Called after IRQ completes (TxDone, RxDone, Timeout).
 *
 * Sets state to fallback mode (STDBY_RC by default).
 * DS Section 13.1.15: SetRxTxFallbackMode.
 */
void sx1262_fsm_on_irq_complete(void);

/**
 * @brief Check if a transition from current state to target is valid.
 *
 * Does NOT modify state. For query/validation only.
 */
bool sx1262_fsm_can_transition(sx1262_state_t target);

#ifdef __cplusplus
}
#endif

#endif // SX1262_FSM_H
