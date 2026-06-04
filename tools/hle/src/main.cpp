#include <thread>
#include <cstdio>
#include <cstdlib>
#include <ctime>

extern "C" {
#include "esp_log.h"
}
#include "hle/sdl_renderer.h"
#include "hle/hle_display.h"
#include "hle/lv_port_hle.h"

#include <lvgl.h>

static const char *TAG = "MAIN";

static uint32_t lv_get_tick(void) {
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    hle::SDLRenderer renderer;
    if (!renderer.init()) {
        fprintf(stderr, "Failed to initialize SDL2\n");
        return 1;
    }

    lv_init();
    lv_tick_set_cb(lv_get_tick);

    hle_lv_port_disp_init();

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "TentacleOS HLE\n\nPress ESC to quit");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "OK");

    ESP_LOGI(TAG, "Entering main loop...");

    bool quit = false;
    while (!quit) {
        uint8_t keys_pressed, keys_held;
        renderer.handle_events(keys_pressed, keys_held, quit);

        lv_timer_handler();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        renderer.render();
    }

    renderer.shutdown();
    ESP_LOGI(TAG, "Shutdown complete");
    return 0;
}
