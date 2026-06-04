#include <lvgl.h>
#include <thread>
#include <chrono>

#include "hle/spi_bridge_channel.h"

extern "C" {
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hle/lv_port_hle.h"
}

static const char *TAG = "HLE_KERNEL";
static hle::SPIBridgeChannel *s_channel = nullptr;

void hle_set_bridge_channel(void *ch) {
    s_channel = static_cast<hle::SPIBridgeChannel *>(ch);
    ::hle_set_bridge_channel(s_channel);
}

static void c5_bridge_worker(void) {
    ESP_LOGI(TAG, "C5 bridge worker started");
    while (s_channel) {
        uint8_t cmd_id, payload[256], payload_len;
        if (!s_channel->slave_wait_command(cmd_id, payload, payload_len)) break;

        switch (cmd_id) {
        case 0x01: // SYSTEM_PING
            s_channel->slave_send_response(cmd_id, 0x00, nullptr, 0);
            break;
        case 0x02: { // SYSTEM_STATUS
            uint8_t status[] = {0x01, 0x00, 0x01, 0x00, 0x05};
            s_channel->slave_send_response(cmd_id, 0x00, status, sizeof(status));
            break;
        }
        case 0x03: // SYSTEM_VERSION
            s_channel->slave_send_response(cmd_id, 0x00, (const uint8_t *)"HLE v1.0.0", 10);
            break;
        case 0x20: { // WIFI_APP_SCAN
            uint8_t count[2] = {3, 0};
            s_channel->slave_send_response(cmd_id, 0x00, count, 2);
            break;
        }
        default:
            s_channel->slave_send_response(cmd_id, 0x01, nullptr, 0);
            break;
        }
        s_channel->slave_notify_irq();
    }
}

static void create_boot_screen(void) {
    lv_obj_t *scr = lv_screen_active();

    lv_obj_t *logo = lv_label_create(scr);
    lv_label_set_text(logo, "TentacleOS");
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_48, 0);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *ver = lv_label_create(scr);
    lv_label_set_text(ver, "HIGH BOY\nv1.0.0-HLE");
    lv_obj_set_style_text_align(ver, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ver, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *loading = lv_label_create(scr);
    lv_label_set_text(loading, "Loading...");
    lv_obj_align(loading, LV_ALIGN_BOTTOM_MID, 0, -40);
}

static void create_home_screen(void) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t *scr = lv_screen_active();

    lv_obj_t *status = lv_label_create(scr);
    lv_label_set_text(status, "WiFi: OFF  BT: ON  Batt: 75%");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 4);

    static const char *items[] = {
        "WiFi Scanner", "BLE Scanner", "SubGHz Spectrum",
        "BadUSB", "NFC", "Settings", "About"
    };

    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_button_create(scr);
        lv_obj_set_size(btn, 200, 32);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, -60 + i * 36);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i]);
        lv_obj_center(label);
    }
}

extern "C" void hle_kernel_init(void) {
    ESP_LOGI(TAG, "HLE Kernel initializing...");

    nvs_flash_init();
    hle_lv_port_disp_init();

    create_boot_screen();
    ESP_LOGI(TAG, "Boot screen shown");

    lv_timer_handler();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    create_home_screen();
    ESP_LOGI(TAG, "Home screen shown");

    if (s_channel) {
        xTaskCreate([](void *) { c5_bridge_worker(); vTaskDelete(nullptr); },
                    "c5_bridge", 8192, nullptr, 5, nullptr);
        ESP_LOGI(TAG, "C5 bridge worker started");
    }

    ESP_LOGI(TAG, "HLE Kernel boot complete");
}
