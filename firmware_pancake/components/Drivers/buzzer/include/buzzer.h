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
 * @file buzzer.h
 * @brief Passive piezo buzzer driver for Pancake (LEDC PWM tone, GPIO6).
 *
 * A passive piezo needs a driven square wave, not a static level, so tones
 * are produced with an LEDC PWM channel whose frequency is the tone pitch.
 */

#ifndef BUZZER_H
#define BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/** @brief Initialize the buzzer LEDC timer/channel. Silent until a tone. */
void buzzer_init(void);

/** @brief Globally enable/disable the buzzer (e.g. from the sound settings/NVS). */
void buzzer_set_enabled(bool enabled);

/** @brief Whether the buzzer is currently enabled. */
bool buzzer_is_enabled(void);

/**
 * @brief Play a blocking tone.
 *
 * @param freq_hz  Tone frequency in Hz (e.g. 2000-5000 for a click).
 * @param ms       Duration in milliseconds.
 */
void buzzer_tone(uint32_t freq_hz, uint32_t ms);

/** @brief Short UI feedback click. */
void buzzer_click(void);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
