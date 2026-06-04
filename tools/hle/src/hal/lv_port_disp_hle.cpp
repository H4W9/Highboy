extern "C" {
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
}

#include "hle/hle_display.h"
#include <lvgl.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

static const char *TAG = "LV_PORT_DISP";

#define LCD_H_RES 240
#define LCD_V_RES 320

static lv_display_t *s_disp = nullptr;

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    // LVGL uses RGB565 already
    hle::Display::instance().draw_bitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1,
                                          reinterpret_cast<const uint16_t *>(px_map));

    lv_display_flush_ready(disp);
}

extern "C" {

void lv_port_disp_init(void);
void hle_lv_port_disp_init(void);

void lv_port_disp_init(void) {
    s_disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(s_disp, disp_flush_cb);

    size_t buf_size = LCD_H_RES * LCD_V_RES / 2 * sizeof(lv_color_t);
    auto *buf1 = static_cast<lv_color_t *>(malloc(buf_size));
    auto *buf2 = static_cast<lv_color_t *>(malloc(buf_size));
    lv_display_set_buffers(s_disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "HLE display initialized (%dx%d)", LCD_H_RES, LCD_V_RES);
}

void hle_lv_port_disp_init(void) { lv_port_disp_init(); }

} // extern "C"
