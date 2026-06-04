#include <lvgl.h>
#include <thread>
#include <chrono>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <unordered_set>
#include <mutex>

#include "hle/spi_bridge_channel.h"

extern "C" {
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hle/lv_port_hle.h"
}

static const char *TAG = "HLE_SHIM";
static hle::SPIBridgeChannel *s_channel = nullptr;

void hle_set_bridge_channel(void *ch) {
    s_channel = static_cast<hle::SPIBridgeChannel *>(ch);
    ::hle_set_bridge_channel(s_channel);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Hardware init stubs (their .c files need deep ESP-IDF integration)
// ═══════════════════════════════════════════════════════════════════════════════

extern "C" {

void ota_post_boot_check(void) {}
void wifi_service_init(void) {}
void console_service_init(void) {}
void bridge_manager_init(void) {}
void cc1101_init(void) {}
void ys_rfid2_init(const void *cfg) { (void)cfg; }
void led_rgb_init(void) {}
void tos_log_init(void) {}
void tos_first_boot_setup(void) {}

// ── Stubs for excluded hardware-specific implementation files ───────────────
// (bluetooth_service.c, wifi_service.c, console/cmds, etc. are excluded
//  because they need full NimBLE/lwIP host stacks)

bool wifi_service_is_active(void) { return false; }
bool wifi_service_is_connected(void) { return false; }
void wifi_service_set_enabled(bool en) { (void)en; }

void ledc_timer_config(const void *cfg) { (void)cfg; }
void ledc_channel_config(const void *cfg) { (void)cfg; }
void ledc_set_duty(int mode, int chan, int timer, int duty) { (void)mode; (void)chan; (void)timer; (void)duty; }
void ledc_update_duty(int mode, int chan, int timer, int duty) { (void)mode; (void)chan; (void)timer; (void)duty; }

void bad_usb_init(void) {}
void bad_usb_deinit(void) {}
void bad_usb_wait_for_connection(void) {}
void ducky_set_layout(const void *layout) { (void)layout; }
void ducky_set_progress_callback(void *cb, void *ctx) { (void)cb; (void)ctx; }
void ducky_abort(void) {}
void ducky_run_from_assets(const char *path) { (void)path; }

void ui_connect_bt_open(void) {}
void ui_connect_wifi_open(void) {}
void ui_ble_menu_open(void) {}
void ui_ble_spam_open(void) {}
void ui_ble_spam_select_open(void) {}
void ui_nfc_menu_open(void) {}
void ui_subghz_spectrum_open(void) {}
void ui_wifi_ap_list_open(void) {}
void ui_wifi_attack_menu_open(void) {}
void ui_wifi_auth_flood_open(void) {}
void ui_wifi_beacon_spam_open(void) {}
void ui_wifi_beacon_spam_simple_open(void) {}
void ui_wifi_deauth_attack_open(void) {}
void ui_wifi_deauth_open(void) {}
void ui_wifi_evil_twin_open(void) {}
void ui_wifi_menu_open(void) {}
void ui_wifi_packets_menu_open(void) {}
void ui_wifi_probe_flood_open(void) {}
void ui_wifi_probe_open(void) {}
void ui_wifi_scan_ap_open(void) {}
void ui_wifi_scan_menu_open(void) {}
void ui_wifi_scan_monitor_open(void) {}
void ui_wifi_scan_open(void) {}
void ui_wifi_scan_probe_open(void) {}
void ui_wifi_scan_stations_open(void) {}
void ui_wifi_scan_target_open(void) {}
void ui_wifi_sniffer_attack_open(void) {}
void ui_wifi_sniffer_handshake_open(void) {}
void ui_wifi_sniffer_raw_open(void) {}
void ui_badusb_menu_open(void) {}
void ui_badusb_browser_open(void) {}
void ui_badusb_layout_open(void) {}
void ui_badusb_connect_open(void) {}
void ui_badusb_running_open(void) {}
void ui_files_open(void) {}
void ui_menu_open(void) {}
void ui_settings_open(void) {}
void ui_display_settings_open(void) {}
void ui_interface_settings_open(void) {}
void ui_sound_settings_open(void) {}
void ui_battery_settings_open(void) {}
void ui_connection_settings_open(void) {}
void ui_about_settings_open(void) {}

// VFS backend stubs (mount host tmpdir)
static bool s_vfs_mounted = false;
bool vfs_sdcard_is_mounted(void) { return s_vfs_mounted; }
void vfs_register_sd_backend(void) {
    std::string dir = "/tmp/hle_sdcard";
    mkdir(dir.c_str(), 0755);
    s_vfs_mounted = true;
    fprintf(stderr, "I [VFS] HLE SD card mounted at %s\n", dir.c_str());
}
void vfs_unregister_sd_backend(void) { s_vfs_mounted = false; }

void ui_theme_selector_open(void) {}
void ui_ir_menu_open(void) {}
void ui_ir_receive_open(void) {}
void ui_ir_send_open(void) {}
void ui_ir_controller_open(void) {}
void ui_ir_saved_open(void) {}
void ui_ir_burst_open(void) {}

// UI component stubs
void toggle_ui_create(void *parent, const char *label, bool *val, void *cb) {
    (void)parent; (void)label; (void)val; (void)cb;
}
bool toggle_ui_get(void *toggle) { (void)toggle; return false; }
void toggle_ui_toggle(void *toggle) { (void)toggle; }
void page_dots_create(void *parent, int count) { (void)parent; (void)count; }
void page_dots_set(void *dots, int idx) { (void)dots; (void)idx; }
void page_dots_show(void *dots) { (void)dots; }
void page_dots_hide(void *dots) { (void)dots; }

// Recursive mutex — stored separately from regular SemaphoreInfo,
// vSemaphoreDelete in esp_shim.cpp handles cleanup for both types.
extern "C" void _hle_vSemaphoreDelete(void *sem);
static std::mutex s_rmutex_set_mtx;
static std::unordered_set<pthread_mutex_t *> s_rmutex_set;

static pthread_mutex_t *hle_rmutex_create(void) {
    auto *m = new pthread_mutex_t;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &attr);
    pthread_mutexattr_destroy(&attr);
    {
        std::lock_guard<std::mutex> lock(s_rmutex_set_mtx);
        s_rmutex_set.insert(m);
    }
    return m;
}
static bool hle_rmutex_lock(pthread_mutex_t *m) { return pthread_mutex_lock(m) == 0; }
static bool hle_rmutex_unlock(pthread_mutex_t *m) { return pthread_mutex_unlock(m) == 0; }

SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void) {
    return (SemaphoreHandle_t)hle_rmutex_create();
}
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t m, TickType_t timeout) {
    (void)timeout;
    return hle_rmutex_lock((pthread_mutex_t *)m) ? pdTRUE : pdFALSE;
}
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t m) {
    return hle_rmutex_unlock((pthread_mutex_t *)m) ? pdTRUE : pdFALSE;
}

// Override vSemaphoreDelete to handle recursive mutexes as well
extern "C" void _hle_vSemaphoreDelete(void *sem);
void _hle_vSemaphoreDelete(void *sem) {
    if (!sem) return;
    {
        std::lock_guard<std::mutex> lock(s_rmutex_set_mtx);
        auto it = s_rmutex_set.find((pthread_mutex_t *)sem);
        if (it != s_rmutex_set.end()) {
            pthread_mutex_destroy(*it);
            delete *it;
            s_rmutex_set.erase(it);
            return;
        }
    }
    vSemaphoreDelete(sem);
}

// Bluetooth service init
void bluetooth_service_init(void) {}
void bluetooth_service_start(void) {}
void bluetooth_service_stop(void) {}

// Dispatcher stubs (dispatcher source files excluded — routing tables)
uint8_t wifi_dispatcher_execute(uint8_t cmd, const uint8_t *p, uint8_t pl, uint8_t *rp, uint8_t *rl) {
    (void)cmd; (void)p; (void)pl; (void)rp;
    if (rl) *rl = 0;
    return 0x01; // SPI_STATUS_UNSUPPORTED
}
uint8_t bt_dispatcher_execute(uint8_t cmd, const uint8_t *p, uint8_t pl, uint8_t *rp, uint8_t *rl) {
    (void)cmd; (void)p; (void)pl; (void)rp;
    if (rl) *rl = 0;
    return 0x01;
}

// sys_monitor needed
void uxTaskGetSystemState(void *tasks, uint32_t count, uint32_t *run_time) {
    (void)tasks; (void)count;
    if (run_time) *run_time = 0;
}
void bluetooth_service_scan(uint32_t timeout) { (void)timeout; }
int bluetooth_service_get_scan_count(void) { return 0; }
void bluetooth_service_get_scan_result(int idx, void *out) { (void)idx; (void)out; }
void bluetooth_service_connect(const void *addr) { (void)addr; }
void bluetooth_service_disconnect_all(void) {}
void bluetooth_service_get_mac(uint8_t *mac) { memset(mac, 0, 6); }
void ble_scanner_start(void *cb, void *ctx) { (void)cb; (void)ctx; }
void ble_scanner_stop(void) {}
bool ble_scanner_is_running(void) { return false; }
void ble_sniffer_start(void *cb, void *ctx) { (void)cb; (void)ctx; }
void ble_sniffer_stop(void) {}
void ble_sniffer_bind_session(uint32_t id) { (void)id; }
void ble_sniffer_session_killed(void) {}
void ble_connect_flood_start(void *cfg) { (void)cfg; }
void ble_connect_flood_stop(void) {}
void skimmer_detector_start(void *cb, void *ctx) { (void)cb; (void)ctx; }
void skimmer_detector_stop(void) {}
void tracker_detector_start(void *cb, void *ctx) { (void)cb; (void)ctx; }
void tracker_detector_stop(void) {}
void meshtastic_transport_init(void) {}
void meshtastic_gatt_init(void *cb) { (void)cb; }
void meshtastic_gatt_stop(void) {}
void meshtastic_transport_inject_fromradio_chunk(const uint8_t *d, size_t l) { (void)d; (void)l; }
void meshtastic_transport_inject_log_chunk(const uint8_t *d, size_t l) { (void)d; (void)l; }
void meshtastic_transport_get_status(void *s) { (void)s; }
void meshcore_transport_init(void) {}
void meshcore_gatt_init(void *cb) { (void)cb; }
void meshcore_gatt_stop(void) {}
void meshcore_transport_inject_tx_chunk(const uint8_t *d, size_t l) { (void)d; (void)l; }

// ESP-IDF stubs
UBaseType_t uxTaskGetNumberOfTasks(void) { return 1; }

// ESP-IDF stubs
const char *esp_err_to_name(int err) { (void)err; return "HLE_ERR"; }
uint32_t esp_get_free_heap_size(void) { return 8 * 1024 * 1024; }
void spi_bus_free(int host) { (void)host; }
esp_err_t esp_littlefs_info(const char *label, size_t *total, size_t *used) {
    if (total) *total = 4 * 1024 * 1024;
    if (used) *used = 0;
    return ESP_OK;
}

} // extern "C"

// ESP_ERROR_CHECK macro — needed by kernel.c
extern "C" void _esp_error_check_failed(int rc, const char *file, int line, const char *func, const char *expr) {
    if (rc != 0) {
        fprintf(stderr, "E [%s:%d] %s(): ESP_ERROR_CHECK failed: %s (err=0x%x)\n", file, line, func, expr, rc);
        exit(1);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// C5 Bridge Worker (runs real spi_bridge.c logic in background)
// ═══════════════════════════════════════════════════════════════════════════════

extern "C" {

static void c5_bridge_worker(void) {
    ESP_LOGI(TAG, "C5 bridge worker started");
    while (s_channel) {
        uint8_t cmd_id, payload[256], payload_len;
        if (!s_channel->slave_wait_command(cmd_id, payload, payload_len)) break;

        switch (cmd_id) {
        case 0x01: s_channel->slave_send_response(cmd_id, 0x00, nullptr, 0); break;
        case 0x02: { uint8_t s[] = {1,0,1,0,5}; s_channel->slave_send_response(cmd_id, 0x00, s, 5); break; }
        case 0x03:  s_channel->slave_send_response(cmd_id, 0x00, (const uint8_t*)"HLE", 3); break;
        default:    s_channel->slave_send_response(cmd_id, 0x01, nullptr, 0); break;
        }
        s_channel->slave_notify_irq();
    }
}

} // extern "C"

// ═══════════════════════════════════════════════════════════════════════════════
// Public API: boots the REAL firmware kernel + UI
// ═══════════════════════════════════════════════════════════════════════════════

extern "C" {

// Forward-declare the real firmware entry point
extern void kernel_init(void);

void hle_kernel_init(void) {
    if (s_channel) {
        xTaskCreate([](void *) { c5_bridge_worker(); vTaskDelete(nullptr); },
                    "c5_bridge", 8192, nullptr, 5, nullptr);
    }

    ESP_LOGI("MAIN", "Booting TentacleOS firmware...");
    kernel_init();
    ESP_LOGI("MAIN", "Firmware booted — running LVGL event loop");
}

} // extern "C"
