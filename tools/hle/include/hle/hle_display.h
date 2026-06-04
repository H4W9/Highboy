#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
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

    void draw_bitmap_strided(int x0, int y0, int x1, int y1, const uint8_t *data, size_t src_stride_bytes) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const int w = x1 - x0;
        const int h = y1 - y0;
        if (w <= 0 || h <= 0) return;

        for (int row = 0; row < h; ++row) {
            const auto *src_row = reinterpret_cast<const uint16_t *>(data + (row * src_stride_bytes));
            for (int col = 0; col < w; ++col) {
                const int x = x0 + col;
                const int y = y0 + row;
                const int idx = y * LCD_H_RES + x;
                if (idx >= 0 && idx < LCD_H_RES * LCD_V_RES) {
                    m_pixels[idx] = src_row[col];
                }
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

    bool save_ppm(const char *path) {
        if (path == nullptr) {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        FILE *file = fopen(path, "wb");
        if (file == nullptr) {
            return false;
        }

        fprintf(file, "P6\n%d %d\n255\n", LCD_H_RES, LCD_V_RES);
        for (int i = 0; i < LCD_H_RES * LCD_V_RES; ++i) {
            const uint16_t pixel = m_pixels[i];
            const uint8_t r = static_cast<uint8_t>(((pixel >> 11) & 0x1F) * 255 / 31);
            const uint8_t g = static_cast<uint8_t>(((pixel >> 5) & 0x3F) * 255 / 63);
            const uint8_t b = static_cast<uint8_t>((pixel & 0x1F) * 255 / 31);
            fputc(r, file);
            fputc(g, file);
            fputc(b, file);
        }

        fclose(file);
        return true;
    }

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
