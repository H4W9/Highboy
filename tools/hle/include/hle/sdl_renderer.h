#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

#include "hle/hle_display.h"

namespace hle {

class SDLRenderer {
public:
    SDLRenderer(int scale = 3) : m_scale(scale) {}

    ~SDLRenderer() { shutdown(); }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return false;
        }

        m_window = SDL_CreateWindow("TentacleOS HLE",
                                     SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                     LCD_H_RES * m_scale, LCD_V_RES * m_scale,
                                     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!m_window) {
            fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
            return false;
        }

        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        if (!m_renderer) {
            m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
        }
        if (!m_renderer) {
            fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
            return false;
        }

        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB565,
                                       SDL_TEXTUREACCESS_STREAMING, LCD_H_RES, LCD_V_RES);
        if (!m_texture) {
            fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
            return false;
        }

        SDL_RenderSetLogicalSize(m_renderer, LCD_H_RES, LCD_V_RES);
        return true;
    }

    bool handle_events(uint8_t &keys_pressed, uint8_t &keys_held, bool &quit) {
        quit = false;
        keys_pressed = 0;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                quit = true;
                return true;
            case SDL_KEYDOWN:
                if (ev.key.repeat) break;
                keys_pressed |= sdl_key_to_hle(ev.key.keysym.sym);
                break;
            case SDL_KEYUP:
                m_held_keys &= ~sdl_key_to_hle(ev.key.keysym.sym);
                break;
            }
        }
        // Track held keys
        const uint8_t *state = SDL_GetKeyboardState(nullptr);
        m_held_keys = 0;
        if (state[SDL_SCANCODE_UP]) m_held_keys |= KEY_UP;
        if (state[SDL_SCANCODE_DOWN]) m_held_keys |= KEY_DOWN;
        if (state[SDL_SCANCODE_LEFT]) m_held_keys |= KEY_LEFT;
        if (state[SDL_SCANCODE_RIGHT]) m_held_keys |= KEY_RIGHT;
        if (state[SDL_SCANCODE_RETURN]) m_held_keys |= KEY_OK;
        if (state[SDL_SCANCODE_BACKSPACE]) m_held_keys |= KEY_BACK;
        keys_held = m_held_keys;
        return true;
    }

    void render() {
        auto &disp = Display::instance();
        std::lock_guard<std::mutex> lock(disp.mutex());

        void *pixels;
        int pitch;
        SDL_LockTexture(m_texture, nullptr, &pixels, &pitch);
        const auto *src = reinterpret_cast<const uint8_t *>(disp.pixels());
        auto *dst = reinterpret_cast<uint8_t *>(pixels);
        const size_t row_bytes = LCD_H_RES * sizeof(uint16_t);
        for (int y = 0; y < LCD_V_RES; ++y) {
            memcpy(dst + (y * pitch), src + (y * row_bytes), row_bytes);
        }
        SDL_UnlockTexture(m_texture);

        SDL_RenderClear(m_renderer);
        SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
        SDL_RenderPresent(m_renderer);
    }

    void shutdown() {
        if (m_texture) SDL_DestroyTexture(m_texture);
        if (m_renderer) SDL_DestroyRenderer(m_renderer);
        if (m_window) SDL_DestroyWindow(m_window);
        m_texture = nullptr;
        m_renderer = nullptr;
        m_window = nullptr;
        SDL_Quit();
    }

    uint8_t held_keys() const { return m_held_keys; }

    // Button bits matching firmware
    static constexpr uint8_t KEY_UP    = 1 << 0;
    static constexpr uint8_t KEY_DOWN  = 1 << 1;
    static constexpr uint8_t KEY_LEFT  = 1 << 2;
    static constexpr uint8_t KEY_RIGHT = 1 << 3;
    static constexpr uint8_t KEY_OK    = 1 << 4;
    static constexpr uint8_t KEY_BACK  = 1 << 5;

private:
    static uint8_t sdl_key_to_hle(SDL_Keycode sym) {
        switch (sym) {
        case SDLK_UP: return KEY_UP;
        case SDLK_DOWN: return KEY_DOWN;
        case SDLK_LEFT: return KEY_LEFT;
        case SDLK_RIGHT: return KEY_RIGHT;
        case SDLK_RETURN: return KEY_OK;
        case SDLK_BACKSPACE: return KEY_BACK;
        default: return 0;
        }
    }

    SDL_Window   *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;
    SDL_Texture  *m_texture = nullptr;
    int           m_scale;
    uint8_t       m_held_keys = 0;
};

}  // namespace hle
