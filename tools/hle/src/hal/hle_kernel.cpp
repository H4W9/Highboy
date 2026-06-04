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

// ── Firmware Asset Stubs ─────────────────────────────────────────────────────

extern "C" {

void assets_manager_init(void) {}
void assets_manager_free_all(void) {}
void *assets_get(const char *path) { (void)path; return nullptr; }

// Other stubs needed by ui_manager / kernel
void tos_first_boot_setup(void) {}
void tos_config_load_all(void) {}
void tos_log_init(void) {}
void tos_theme_load_from_sd(void) {}
void storage_init(void) {}
void storage_assets_init(void) {}
void storage_assets_print_info(void) {}
void st7789_init(void) {}
void cc1101_init(void) {}
void bridge_manager_init(void) {}
void ys_rfid2_init(void *ctx) { (void)ctx; }
void wifi_service_init(void) {}
void console_service_init(void) {}
void ui_theme_init(void) {}
void header_ui_create(void *parent) { (void)parent; }
void dropdown_ui_create(void *parent) { (void)parent; }

} // extern "C"

// ── C5 Bridge Worker ─────────────────────────────────────────────────────────

static void c5_bridge_worker(void) {
    ESP_LOGI(TAG, "C5 bridge worker started");
    while (s_channel) {
        uint8_t cmd_id, payload[256], payload_len;
        if (!s_channel->slave_wait_command(cmd_id, payload, payload_len)) break;

        switch (cmd_id) {
        case 0x01:
            s_channel->slave_send_response(cmd_id, 0x00, nullptr, 0);
            break;
        case 0x02: {
            uint8_t status[] = {0x01, 0x00, 0x01, 0x00, 0x05};
            s_channel->slave_send_response(cmd_id, 0x00, status, sizeof(status));
            break;
        }
        case 0x03:
            s_channel->slave_send_response(cmd_id, 0x00, (const uint8_t *)"HLE v1.0.0", 10);
            break;
        default:
            s_channel->slave_send_response(cmd_id, 0x01, nullptr, 0);
            break;
        }
        s_channel->slave_notify_irq();
    }
}

// ── Boot Screen (uses real LVGL, mimics firmware look) ───────────────────────

static void create_boot_screen(void) {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *logo = lv_label_create(scr);
    lv_label_set_text(logo, "TentacleOS");
    lv_obj_set_style_text_color(logo, lv_color_hex(0x00FF66), 0);
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_48, 0);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *sub = lv_label_create(scr);
    lv_label_set_text(sub, "HIGH BOY  •  v1.0.0-HLE");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 115);

    lv_obj_t *loading = lv_label_create(scr);
    lv_label_set_text(loading, "Booting...");
    lv_obj_set_style_text_color(loading, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(loading, &lv_font_montserrat_16, 0);
    lv_obj_align(loading, LV_ALIGN_BOTTOM_MID, 0, -30);
}

// ── Home Screen ──────────────────────────────────────────────────────────────

static void create_home_screen(void) {
    lv_obj_clean(lv_screen_active());
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Status bar
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, 240, 22);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0F3460), 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);

    lv_obj_t *status = lv_label_create(bar);
    lv_label_set_text(status, "WiFi  OFF   BT  ON   75%");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(0xCCCCCC), 0);
    lv_obj_center(status);

    // Menu
    static const char *items[] = {
        "WiFi Scanner", "BLE Scanner", "SubGHz Spectrum",
        "BadUSB", "NFC", "Settings", "About"
    };

    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_button_create(scr);
        lv_obj_set_size(btn, 210, 30);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, -55 + i * 36);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x16213E), 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_center(label);
    }
}

// ── Public API ───────────────────────────────────────────────────────────────

extern "C" void hle_kernel_init(void) {
    ESP_LOGI(TAG, "HLE Kernel initializing...");

    nvs_flash_init();
    hle_lv_port_disp_init();

    // Boot screen
    create_boot_screen();
    lv_timer_handler();
    ESP_LOGI(TAG, "Boot screen shown");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Home screen
    create_home_screen();
    ESP_LOGI(TAG, "Home screen shown");

    // C5 bridge worker
    if (s_channel) {
        xTaskCreate([](void *) { c5_bridge_worker(); vTaskDelete(nullptr); },
                    "c5_bridge", 8192, nullptr, 5, nullptr);
        ESP_LOGI(TAG, "C5 bridge worker started");
    }

    ESP_LOGI(TAG, "HLE Kernel boot complete");
}
