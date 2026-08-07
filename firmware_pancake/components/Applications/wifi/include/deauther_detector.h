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

#ifndef DEAUTHER_DETECTOR_H
#define DEAUTHER_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Start the deauth detector over SPI.
 */
void deauther_detector_start(void);

/**
 * @brief Stop the deauth detector.
 */
void deauther_detector_stop(void);

/**
 * @brief Get the number of deauth frames detected.
 *
 * @return Deauth frame count.
 */
uint32_t deauther_detector_get_count(void);

#ifdef __cplusplus
}
#endif

#endif // DEAUTHER_DETECTOR_H
