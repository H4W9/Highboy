#include <cstdio>
#include <cstring>
#include <string>
#include <cerrno>
#include <sys/stat.h>
#include <pthread.h>
#include <unordered_set>
#include <mutex>

#include "hle/spi_bridge_channel.h"

extern "C" {
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

static const char *TAG = "HLE_SHIM";
static hle::SPIBridgeChannel *s_channel = nullptr;

void hle_set_bridge_channel(void *ch) {
    s_channel = static_cast<hle::SPIBridgeChannel *>(ch);
    ::hle_set_bridge_channel(s_channel);
}

// ═══════════════════════════════════════════════════════════════════════════════
// VFS backend — mounts host tmpdir as SD card
// ═══════════════════════════════════════════════════════════════════════════════

extern "C" {

static bool s_vfs_mounted = false;
bool vfs_sdcard_is_mounted(void) { return s_vfs_mounted; }
esp_err_t vfs_register_sd_backend(void) {
    std::string base = "/tmp/hle_storage";
    std::string dir = base + "/sdcard";
    if ((mkdir(base.c_str(), 0755) != 0 && errno != EEXIST) ||
        (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST)) {
        ESP_LOGE(TAG, "Failed to create HLE storage path: %s", dir.c_str());
        s_vfs_mounted = false;
        return ESP_FAIL;
    }
    s_vfs_mounted = true;
    ESP_LOGI(TAG, "HLE SD card mounted at %s", dir.c_str());
    return ESP_OK;
}
esp_err_t vfs_unregister_sd_backend(void) {
    s_vfs_mounted = false;
    return ESP_OK;
}

} // extern "C"

// ═══════════════════════════════════════════════════════════════════════════════
// Recursive mutex — FreeRTOS recursive semaphore emulation via pthread
// ═══════════════════════════════════════════════════════════════════════════════

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

extern "C" {

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

void uxTaskGetSystemState(void *tasks, uint32_t count, uint32_t *run_time) {
    (void)tasks; (void)count;
    if (run_time) *run_time = 0;
}
UBaseType_t uxTaskGetNumberOfTasks(void) { return 1; }

} // extern "C"

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

// ═══════════════════════════════════════════════════════════════════════════════
// C5 Bridge Worker (simulates C5 co-processor SPI responses)
// ═══════════════════════════════════════════════════════════════════════════════

extern "C" {

#include "ota_version.h"

#define HLE_CMD_PING   0x01
#define HLE_CMD_STATUS 0x02
#define HLE_CMD_VERSION 0x03
#define SPI_STATUS_OK          0x00
#define SPI_STATUS_UNSUPPORTED 0x03

static void c5_bridge_worker(void) {
    ESP_LOGI(TAG, "C5 bridge worker started");
    while (s_channel) {
        uint8_t cmd_id, payload[256], payload_len;
        if (!s_channel->slave_wait_command(cmd_id, payload, payload_len)) break;

        switch (cmd_id) {
        case HLE_CMD_PING: s_channel->slave_send_response(cmd_id, SPI_STATUS_OK, nullptr, 0); break;
        case HLE_CMD_STATUS: { uint8_t s[] = {1,0,1,0,5}; s_channel->slave_send_response(cmd_id, SPI_STATUS_OK, s, 5); break; }
        case HLE_CMD_VERSION: s_channel->slave_send_response(cmd_id, SPI_STATUS_OK,
            reinterpret_cast<const uint8_t *>(FIRMWARE_VERSION),
            static_cast<uint8_t>(strlen(FIRMWARE_VERSION))); break;
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
