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

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief Initialize the LVGL display driver.
 *
 * Creates a display instance backed by two DMA-capable draw buffers
 * (partial rendering, 1/5 of screen each). Registers the flush
 * callback that sends pixels to the ST7789 panel via esp_lcd, and
 * optionally mirrors frames to the BLE screen server.
 */
void lv_port_disp_init(void);

#ifdef __cplusplus
}
#endif

#endif // LV_PORT_DISP_H
