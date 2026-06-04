#pragma once

#include <cstdint>
#include <cstring>
#include <mutex>

namespace hle {

static constexpr int LCD_H_RES = 240;
static constexpr int LCD_V_RES = 320;

class Display {
public:
    Display() : m_pixels(new uint16_t[LCD_H_RES * LCD_V_RES]()) {}

    ~Display() { delete[] m_pixels; }

    void draw_bitmap(int x0, int y0, int x1, int y1, const uint16_t *data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int w = x1 - x0;
        if (w <= 0) return;
        for (int y = y0; y < y1; y++) {
            int src_row = (y - y0) * w;
            for (int x = x0; x < x1; x++) {
                int idx = y * LCD_H_RES + x;
                if (idx >= 0 && idx < LCD_H_RES * LCD_V_RES)
                    m_pixels[idx] = data[src_row + (x - x0)];
            }
        }
        m_dirty = true;
    }

    void fill_screen(uint16_t color) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) m_pixels[i] = color;
        m_dirty = true;
    }

    const uint16_t *pixels() const { return m_pixels; }
    int width() const { return LCD_H_RES; }
    int height() const { return LCD_V_RES; }

    bool is_dirty() const { return m_dirty; }
    void clear_dirty() { m_dirty = false; }

    std::mutex &mutex() { return m_mutex; }

    static Display &instance() {
        static Display s_inst;
        return s_inst;
    }

private:
    uint16_t *m_pixels;
    bool m_dirty = true;
    std::mutex m_mutex;
};

}  // namespace hle
