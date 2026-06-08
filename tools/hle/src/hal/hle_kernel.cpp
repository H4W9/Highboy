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

esp_err_t ota_post_boot_check(void) { return ESP_OK; }
esp_err_t console_service_init(void) { return ESP_OK; }
esp_err_t bridge_manager_init(void) { return ESP_OK; }
esp_err_t ys_rfid2_init(const void *cfg) { (void)cfg; return ESP_OK; }
esp_err_t tos_log_init(void) { return ESP_OK; }
esp_err_t tos_first_boot_setup(void) { return ESP_OK; }

esp_err_t ledc_timer_config(const void *cfg) { (void)cfg; return ESP_OK; }
esp_err_t ledc_channel_config(const void *cfg) { (void)cfg; return ESP_OK; }
esp_err_t ledc_set_duty(int mode, int chan, int duty) { (void)mode; (void)chan; (void)duty; return ESP_OK; }
esp_err_t ledc_update_duty(int mode, int chan) { (void)mode; (void)chan; return ESP_OK; }

esp_err_t bad_usb_init(void) { return ESP_OK; }
esp_err_t bad_usb_deinit(void) { return ESP_OK; }
void bad_usb_wait_for_connection(void) {}
void ducky_set_layout(int layout) { (void)layout; }
void ducky_set_progress_callback(void (*cb)(int, int)) { (void)cb; }
void ducky_abort(void) {}
esp_err_t ducky_run_from_assets(const char *path) { (void)path; return ESP_OK; }

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
void ui_theme_selector_open(void) {}
void ui_ir_menu_open(void) {}
void ui_ir_receive_open(void) {}
void ui_ir_send_open(void) {}
void ui_ir_controller_open(void) {}
void ui_ir_saved_open(void) {}
void ui_ir_burst_open(void) {}

// VFS backend stubs (mount host tmpdir)
static bool s_vfs_mounted = false;
bool vfs_sdcard_is_mounted(void) { return s_vfs_mounted; }
esp_err_t vfs_register_sd_backend(void) {
    std::string dir = "/tmp/hle_sdcard";
    mkdir(dir.c_str(), 0755);
    s_vfs_mounted = true;
    fprintf(stderr, "I [VFS] HLE SD card mounted at %s\n", dir.c_str());
    return ESP_OK;
}
esp_err_t vfs_unregister_sd_backend(void) {
    s_vfs_mounted = false;
    return ESP_OK;
}

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

// Dispatcher stubs — shadow C5 compiled dispatchers to prevent pulling in
// C5 application object files (ap_scanner, beacon_spam, etc.) which have
// unresolved ESP-IDF API dependencies. The HLE uses c5_bridge_worker instead.
// Types from spi_protocol.h (not included — HLE shim has no firmware include path).
typedef uint8_t spi_id_t;
typedef uint8_t spi_status_t;
#define SPI_STATUS_OK          0x00
#define SPI_STATUS_UNSUPPORTED 0x03

spi_status_t wifi_dispatcher_execute(spi_id_t id, const uint8_t *payload, uint8_t len,
                                     uint8_t *out_resp_payload, uint8_t *out_resp_len) {
    (void)id; (void)payload; (void)len; (void)out_resp_payload;
    if (out_resp_len) *out_resp_len = 0;
    return SPI_STATUS_UNSUPPORTED;
}
spi_status_t bt_dispatcher_execute(spi_id_t id, const uint8_t *payload, uint8_t len,
                                   uint8_t *out_resp_payload, uint8_t *out_resp_len) {
    (void)id; (void)payload; (void)len; (void)out_resp_payload;
    if (out_resp_len) *out_resp_len = 0;
    return SPI_STATUS_UNSUPPORTED;
}

// sys_monitor needed
void uxTaskGetSystemState(void *tasks, uint32_t count, uint32_t *run_time) {
    (void)tasks; (void)count;
    if (run_time) *run_time = 0;
}
esp_err_t meshtastic_transport_init(void) { return ESP_OK; }
esp_err_t meshtastic_gatt_init(uint32_t node_num) { (void)node_num; return ESP_OK; }
void meshtastic_gatt_stop(void) {}
void meshtastic_transport_inject_fromradio_chunk(const uint8_t *d, uint8_t l) { (void)d; (void)l; }
void meshtastic_transport_inject_log_chunk(const uint8_t *d, uint8_t l) { (void)d; (void)l; }
void meshtastic_transport_get_status(void *s) { (void)s; }
esp_err_t meshcore_transport_init(void) { return ESP_OK; }
esp_err_t meshcore_gatt_init(const char *name_prefix, uint32_t pin) {
    (void)name_prefix; (void)pin;
    return ESP_OK;
}
void meshcore_gatt_stop(void) {}
void meshcore_transport_inject_tx_chunk(const uint8_t *d, uint8_t l) { (void)d; (void)l; }

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

#define HLE_CMD_PING   0x01
#define HLE_CMD_STATUS 0x02
#define HLE_CMD_VERSION 0x03

static void c5_bridge_worker(void) {
    ESP_LOGI(TAG, "C5 bridge worker started");
    while (s_channel) {
        uint8_t cmd_id, payload[256], payload_len;
        if (!s_channel->slave_wait_command(cmd_id, payload, payload_len)) break;

        switch (cmd_id) {
        case HLE_CMD_PING: s_channel->slave_send_response(cmd_id, SPI_STATUS_OK, nullptr, 0); break;
        case HLE_CMD_STATUS: { uint8_t s[] = {1,0,1,0,5}; s_channel->slave_send_response(cmd_id, SPI_STATUS_OK, s, 5); break; }
        case HLE_CMD_VERSION: s_channel->slave_send_response(cmd_id, SPI_STATUS_OK, (const uint8_t*)"HLE", 3); break;
        default: s_channel->slave_send_response(cmd_id, SPI_STATUS_UNSUPPORTED, nullptr, 0); break;
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
