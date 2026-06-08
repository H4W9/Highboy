#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "esp_err.h"
#include "esp_log.h"
}

extern "C" {

esp_err_t ota_post_boot_check(void) { return ESP_OK; }
esp_err_t console_service_init(void) { return ESP_OK; }

esp_err_t c5_flasher_init(void) { return ESP_ERR_NOT_SUPPORTED; }
void c5_flasher_enter_bootloader(void) {}
void c5_flasher_reset_normal(void) {}
esp_err_t c5_flasher_update(const uint8_t *bin_data, uint32_t bin_size) {
    (void)bin_data; (void)bin_size;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ys_rfid2_init(const void *cfg) { (void)cfg; return ESP_OK; }

esp_err_t bad_usb_init(void) { return ESP_OK; }
esp_err_t bad_usb_deinit(void) { return ESP_OK; }
void bad_usb_wait_for_connection(void) {}
void ducky_set_layout(int layout) { (void)layout; }
void ducky_set_progress_callback(void (*cb)(int, int)) { (void)cb; }
void ducky_abort(void) {}
esp_err_t ducky_run_from_assets(const char *path) { (void)path; return ESP_OK; }

esp_err_t ledc_timer_config(const void *cfg) { (void)cfg; return ESP_OK; }
esp_err_t ledc_channel_config(const void *cfg) { (void)cfg; return ESP_OK; }
esp_err_t ledc_set_duty(int mode, int chan, int duty) { (void)mode; (void)chan; (void)duty; return ESP_OK; }
esp_err_t ledc_update_duty(int mode, int chan) { (void)mode; (void)chan; return ESP_OK; }

const char *esp_err_to_name(int err) {
    switch (err) {
    case ESP_OK: return "ESP_OK";
    case ESP_FAIL: return "ESP_FAIL";
    case ESP_ERR_NO_MEM: return "ESP_ERR_NO_MEM";
    case ESP_ERR_INVALID_ARG: return "ESP_ERR_INVALID_ARG";
    case ESP_ERR_INVALID_STATE: return "ESP_ERR_INVALID_STATE";
    case ESP_ERR_INVALID_SIZE: return "ESP_ERR_INVALID_SIZE";
    case ESP_ERR_INVALID_CRC: return "ESP_ERR_INVALID_CRC";
    case ESP_ERR_TIMEOUT: return "ESP_ERR_TIMEOUT";
    case ESP_ERR_NOT_SUPPORTED: return "ESP_ERR_NOT_SUPPORTED";
    case ESP_ERR_NVS_NO_FREE_PAGES: return "ESP_ERR_NVS_NO_FREE_PAGES";
    case ESP_ERR_NVS_NEW_VERSION_FOUND: return "ESP_ERR_NVS_NEW_VERSION_FOUND";
    default: return "ESP_ERR_UNKNOWN";
    }
}
uint32_t esp_get_free_heap_size(void) { return 8 * 1024 * 1024; }
void spi_bus_free(int host) { (void)host; }
esp_err_t esp_littlefs_info(const char *label, size_t *total, size_t *used) {
    if (total) *total = 4 * 1024 * 1024;
    if (used) *used = 0;
    return ESP_OK;
}

void _esp_error_check_failed(int rc, const char *file, int line, const char *func, const char *expr) {
    if (rc != 0) {
        fprintf(stderr, "E [%s:%d] %s(): ESP_ERROR_CHECK failed: %s (err=0x%x)\n", file, line, func, expr, rc);
        exit(1);
    }
}

} // extern "C"
