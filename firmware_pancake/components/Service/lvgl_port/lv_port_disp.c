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

#include "lv_port_disp.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "driver/spi_common.h"
#include "hal/spi_types.h"

#include "st7789.h"
#include "ble_screen_server.h"

static const char *TAG = "LV_PORT_DISP";

// Pancake (ESP32-C5): after linking WiFi, internal RAM is tight (~109 KB).
// Use small partial draw buffers allocated via spi_bus_dma_memory_alloc(),
// which returns memory that is valid DMA source for the SPI panel host
// (matches the proven JANOS/Pancake config). 24 lines each (~15 KB), x2.
#define LVGL_BUF_LINES  24
#define LVGL_BUF_PIXELS (LCD_H_RES * LVGL_BUF_LINES)

static lv_display_t *s_disp_handle = NULL;

static bool flush_ready_cb(esp_lcd_panel_io_handle_t panel_io,
                           esp_lcd_panel_io_event_data_t *edata,
                           void *user_ctx) {
  lv_display_t *disp = (lv_display_t *)user_ctx;
  lv_display_flush_ready(disp);
  return false;
}

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  uint32_t px_count = w * h;

  lv_draw_sw_rgb565_swap(px_map, px_count);

  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);

  if (ble_screen_server_is_active()) {
    ble_screen_server_send_partial((const uint16_t *)px_map, area->x1, area->y1, w, h);
  }
}

void lv_port_disp_init(void) {
  s_disp_handle = lv_display_create(LCD_H_RES, LCD_V_RES);
  lv_display_set_flush_cb(s_disp_handle, disp_flush);

  size_t buf_size = LVGL_BUF_PIXELS * sizeof(lv_color_t);

  // DMA-capable memory for the LCD SPI host (SPI2). Picks internal or PSRAM
  // as appropriate and guarantees the buffer is a valid DMA source.
  void *buf1 = spi_bus_dma_memory_alloc(SPI2_HOST, buf_size, 0);
  void *buf2 = spi_bus_dma_memory_alloc(SPI2_HOST, buf_size, 0);

  if (buf1 == NULL || buf2 == NULL) {
    ESP_LOGE(TAG, "Failed to allocate display buffers (%u bytes each)", (unsigned)buf_size);
    return;
  }

  ESP_LOGI(TAG, "LVGL buffers @ %p / %p (%u bytes each)", buf1, buf2, (unsigned)buf_size);

  lv_display_set_buffers(s_disp_handle, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

  const esp_lcd_panel_io_callbacks_t cbs = {
      .on_color_trans_done = flush_ready_cb,
  };
  esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, s_disp_handle);

  ESP_LOGI(TAG,
           "Display port initialized (%dx%d, buf: %u bytes x2)",
           LCD_H_RES,
           LCD_V_RES,
           (unsigned)buf_size);
}
