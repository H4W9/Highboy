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

#include "st7789.h"
#include "ble_screen_server.h"

static const char *TAG = "LV_PORT_DISP";

// LVGL renders into XRGB8888 buffers (4 bytes/px) so gradients are interpolated
// at 8 bits per channel; disp_flush then dithers them down to RGB565 in place
// before the panel transfer. Using a quarter of the lines keeps internal DMA RAM
// usage equal to the old RGB565 buffers (4 bytes/px over half as many lines).
#define LVGL_BUF_LINES  (LCD_V_RES / 4)
#define LVGL_BUF_PIXELS (LCD_H_RES * LVGL_BUF_LINES)
#define LVGL_BUF_BYTES  (LVGL_BUF_PIXELS * 4)
#define LVGL_BUF_ALLOC  (MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)

static lv_display_t *s_disp_handle = NULL;

// Ordered 8x8 Bayer threshold matrix (values 0..63), indexed by screen
// coordinates so the dither pattern stays stable across partial flushes.
static const uint8_t s_bayer8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},   {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},  {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},   {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},  {63, 31, 55, 23, 61, 29, 53, 21},
};

static inline uint8_t clamp_u8(int v) {
  return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)v);
}

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

  // Dither in place: the RGB565 output (2 bytes/px) is written into the front of
  // the same buffer. The write cursor always trails the XRGB8888 read cursor
  // (4 bytes/px), so every source pixel is consumed before it can be overwritten.
  const lv_color32_t *src = (const lv_color32_t *)px_map;
  uint16_t *dst = (uint16_t *)px_map;

  for (uint32_t row = 0; row < h; row++) {
    const uint8_t *bayer_row = s_bayer8[(area->y1 + row) & 7];
    const lv_color32_t *src_row = &src[row * w];
    uint16_t *dst_row = &dst[row * w];

    for (uint32_t col = 0; col < w; col++) {
      const lv_color32_t *p = &src_row[col];
      uint8_t t = bayer_row[(area->x1 + col) & 7];

      // Add up to one display LSB of ordered noise before truncating: the
      // R/B channels keep 5 bits (LSB step 8), G keeps 6 bits (step 4).
      uint8_t r = clamp_u8(p->red + ((t * 8) >> 6));
      uint8_t g = clamp_u8(p->green + ((t * 4) >> 6));
      uint8_t b = clamp_u8(p->blue + ((t * 8) >> 6));

      uint16_t rgb565 = ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);

      // ST7789 expects the high byte first.
      dst_row[col] = (uint16_t)((rgb565 << 8) | (rgb565 >> 8));
    }
  }

  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, dst);

  if (ble_screen_server_is_active()) {
    ble_screen_server_send_partial(dst, area->x1, area->y1, w, h);
  }
}

void lv_port_disp_init(void) {
  s_disp_handle = lv_display_create(LCD_H_RES, LCD_V_RES);
  lv_display_set_flush_cb(s_disp_handle, disp_flush);

  void *buf1 = heap_caps_malloc(LVGL_BUF_BYTES, LVGL_BUF_ALLOC);
  void *buf2 = heap_caps_malloc(LVGL_BUF_BYTES, LVGL_BUF_ALLOC);

  if (buf1 == NULL || buf2 == NULL) {
    ESP_LOGE(TAG, "Failed to allocate display buffers (%u bytes each)", (unsigned)LVGL_BUF_BYTES);
    return;
  }

  lv_display_set_buffers(s_disp_handle, buf1, buf2, LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);

  const esp_lcd_panel_io_callbacks_t cbs = {
      .on_color_trans_done = flush_ready_cb,
  };
  esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, s_disp_handle);

  ESP_LOGI(TAG,
           "Display port initialized (%dx%d, buf: %u bytes x2)",
           LCD_H_RES,
           LCD_V_RES,
           (unsigned)LVGL_BUF_BYTES);
}
