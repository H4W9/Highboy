#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>

extern "C" {
#include "esp_log.h"
}
#include "hle/sdl_renderer.h"
#include "hle/hle_display.h"
#include "hle/spi_bridge_channel.h"
#include "hle/hle_kernel.h"

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

    // Create bridge channel and hand it to kernel + shims
    hle::SPIBridgeChannel channel;
    hle_set_bridge_channel(&channel);

    // Boot firmware
    hle_kernel_init();

    ESP_LOGI(TAG, "=== TentacleOS HLE running ===");

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
