#include <lvgl.h>
#include <thread>
#include <chrono>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>

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

// C5 firmware stubs (functions called by C5 dispatchers/bridge)
bool wifi_service_is_active(void) { return false; }
bool wifi_service_is_connected(void) { return false; }
bool bluetooth_service_is_running(void) { return false; }
bool bluetooth_service_is_initialized(void) { return false; }

// WiFi sniffer stubs
int wifi_sniffer_get_packet_count(void) { return 0; }
int wifi_sniffer_get_deauth_count(void) { return 0; }
int wifi_sniffer_get_buffer_usage(void) { return 0; }
bool wifi_sniffer_handshake_captured(void) { return false; }
bool wifi_sniffer_pmkid_captured(void) { return false; }
void wifi_sniffer_session_killed(void) {}
void wifi_sniffer_bind_session(uint32_t id) { (void)id; }
void wifi_deauther_stop(void) {}
void wifi_flood_stop(void) {}
void evil_twin_stop_attack(void) {}
void beacon_spam_stop(void) {}
void deauther_detector_stop(void) {}
void deauther_detector_start(void) {}
int deauther_detector_get_count(void) { return 0; }
void probe_monitor_stop(void) {}
void signal_monitor_stop(void) {}
int signal_monitor_get_rssi(void) { return 0; }
void target_scanner_start(void) {}
void target_scanner_stop(void) {}
bool target_scanner_is_scanning(void) { return false; }
void evil_twin_start_attack(void *cfg) { (void)cfg; }

// VFS backend stubs — mount host tmpdir as SD card
static bool s_vfs_mounted = false;

bool vfs_sdcard_is_mounted(void) { return s_vfs_mounted; }

void vfs_register_sd_backend(void) {
    std::string dir = "/tmp/hle_sdcard";
    mkdir(dir.c_str(), 0755);
    s_vfs_mounted = true;
    fprintf(stderr, "I [VFS] HLE SD card mounted at %s\n", dir.c_str());
}

void vfs_unregister_sd_backend(void) { s_vfs_mounted = false; }

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
