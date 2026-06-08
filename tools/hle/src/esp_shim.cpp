#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <queue>
#include <dirent.h>
#include <chrono>
#include <thread>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/spi_slave.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "hle/spi_bridge_channel.h"
#include "hle/hle_display.h"
#include "hle/hle_kernel.h"
#include "esp_lcd_panel_ops.h"
#include "esp_vfs_fat.h"
#include "esp_littlefs.h"
#include "esp_partition.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

// ═══════════════════════════════════════════════════════════════════════════════
// ESP System
// ═══════════════════════════════════════════════════════════════════════════════

void esp_restart(void) {
    fprintf(stderr, "I [SYSTEM] esp_restart() called — exiting\n");
    exit(0);
}

uint64_t esp_timer_get_time(void) {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *handle) {
    (void)args;
    if (handle) *handle = (void *)0x1;
    return ESP_OK;
}
esp_err_t esp_timer_start_periodic(esp_timer_handle_t handle, uint64_t period_us) {
    (void)handle; (void)period_us; return ESP_OK;
}
esp_err_t esp_timer_stop(esp_timer_handle_t handle) { (void)handle; return ESP_OK; }
esp_err_t esp_timer_delete(esp_timer_handle_t handle) { (void)handle; return ESP_OK; }

// ═══════════════════════════════════════════════════════════════════════════════
// FreeRTOS Implementation (pthread-based)
// ═══════════════════════════════════════════════════════════════════════════════

struct TaskInfo {
    pthread_t thread;
    TaskFunction_t func;
    std::string name;
    volatile bool running = true;
    volatile bool suspended = false;
    void *params;
    SemaphoreHandle_t suspend_sem;
};

static std::mutex s_tasks_mutex;
static std::map<TaskHandle_t, TaskInfo> s_tasks;
static TickType_t s_tick_start_us = 0;

static TickType_t get_tick_count(void) {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    if (s_tick_start_us == 0) s_tick_start_us = us;
    return (us - s_tick_start_us) / 1000;  // ms ticks
}

static void *task_wrapper(void *arg) {
    TaskInfo *info = static_cast<TaskInfo *>(arg);
    info->func(info->params);
    info->running = false;
    return nullptr;
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t func, const char *name, uint32_t stack_depth,
                                    void *params, UBaseType_t prio, TaskHandle_t *out_handle,
                                    BaseType_t core_id) {
    (void)stack_depth; (void)prio; (void)core_id;
    auto *info = new TaskInfo{};
    info->func = func;
    info->name = name ? name : "unnamed";
    info->params = params;
    info->suspend_sem = xSemaphoreCreateBinary();

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // firmware tasks request stack in words; on host use 4KB minimum or 2x requested
    size_t stack_bytes = stack_depth * sizeof(void *);
    if (stack_bytes < 65536) stack_bytes = 65536; // 64KB minimum for host threads
    pthread_attr_setstacksize(&attr, stack_bytes);
    pthread_create(&thread, &attr, task_wrapper, info);
    info->thread = thread;

    {
        std::lock_guard<std::mutex> lock(s_tasks_mutex);
        s_tasks[info] = *info;
    }

    if (out_handle) *out_handle = info;
    return pdPASS;
}

void vTaskDelete(TaskHandle_t handle) {
    if (!handle) return;
    std::lock_guard<std::mutex> lock(s_tasks_mutex);
    auto it = s_tasks.find(handle);
    if (it != s_tasks.end()) {
        it->second.running = false;
        s_tasks.erase(it);
    }
    delete static_cast<TaskInfo *>(handle);
}

void vTaskDelay(TickType_t ticks) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ticks * portTICK_PERIOD_MS));
}

TickType_t xTaskGetTickCount(void) { return get_tick_count(); }
char *pcTaskGetName(TaskHandle_t handle) { (void)handle; return (char *)"shim_task"; }
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return nullptr; }

void vTaskSuspend(TaskHandle_t handle) {
    if (!handle) return;
    auto *info = static_cast<TaskInfo *>(handle);
    info->suspended = true;
    xSemaphoreTake(info->suspend_sem, portMAX_DELAY);
}

void vTaskResume(TaskHandle_t handle) {
    if (!handle) return;
    auto *info = static_cast<TaskInfo *>(handle);
    info->suspended = false;
    xSemaphoreGive(info->suspend_sem);
}

UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t handle) {
    (void)handle;
    return 4096;  // enough for any shim task
}

// ═══════════════════════════════════════════════════════════════════════════════
// FreeRTOS Semaphores
// ═══════════════════════════════════════════════════════════════════════════════

struct SemaphoreInfo {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int max_count;
    bool is_mutex;
};

SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    auto *s = new SemaphoreInfo{};
    s->is_mutex = true;
    s->max_count = 1;
    s->count = 1;
    pthread_mutex_init(&s->mutex, nullptr);
    pthread_cond_init(&s->cond, nullptr);
    return s;
}

SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    auto *s = new SemaphoreInfo{};
    s->is_mutex = false;
    s->max_count = 1;
    s->count = 0;
    pthread_mutex_init(&s->mutex, nullptr);
    pthread_cond_init(&s->cond, nullptr);
    return s;
}

SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t max, UBaseType_t initial) {
    auto *s = new SemaphoreInfo{};
    s->is_mutex = false;
    s->max_count = max;
    s->count = initial;
    pthread_mutex_init(&s->mutex, nullptr);
    pthread_cond_init(&s->cond, nullptr);
    return s;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout) {
    if (!sem) return pdFALSE;
    auto *s = static_cast<SemaphoreInfo *>(sem);
    pthread_mutex_lock(&s->mutex);
    int result = 0;
    while (s->count == 0 && result == 0) {
        if (timeout == portMAX_DELAY) {
            pthread_cond_wait(&s->cond, &s->mutex);
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout / 1000;
            ts.tv_nsec += (timeout % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
            result = pthread_cond_timedwait(&s->cond, &s->mutex, &ts);
        }
    }
    if (result == 0 && s->count > 0) {
        s->count--;
        pthread_mutex_unlock(&s->mutex);
        return pdTRUE;
    }
    pthread_mutex_unlock(&s->mutex);
    return pdFALSE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) {
    if (!sem) return pdFALSE;
    auto *s = static_cast<SemaphoreInfo *>(sem);
    pthread_mutex_lock(&s->mutex);
    if (s->count < s->max_count) {
        s->count++;
    }
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
    return pdTRUE;
}

BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t sem, BaseType_t *woken) {
    (void)woken;
    return xSemaphoreGive(sem);
}

void vSemaphoreDelete(SemaphoreHandle_t sem) {
    if (!sem) return;
    auto *s = static_cast<SemaphoreInfo *>(sem);
    pthread_mutex_destroy(&s->mutex);
    pthread_cond_destroy(&s->cond);
    delete s;
}

// ═══════════════════════════════════════════════════════════════════════════════
// FreeRTOS Queues
// ═══════════════════════════════════════════════════════════════════════════════

struct QueueInfo {
    std::queue<std::vector<uint8_t>> items;
    size_t item_size;
    size_t max_items;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

QueueHandle_t xQueueCreate(UBaseType_t queue_len, UBaseType_t item_size) {
    auto *q = new QueueInfo{};
    q->max_items = queue_len;
    q->item_size = item_size;
    pthread_mutex_init(&q->mutex, nullptr);
    pthread_cond_init(&q->cond, nullptr);
    return q;
}

BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t timeout) {
    if (!queue) return pdFALSE;
    auto *q = static_cast<QueueInfo *>(queue);
    pthread_mutex_lock(&q->mutex);
    int result = 0;
    while (q->items.size() >= q->max_items && result == 0) {
        if (timeout == portMAX_DELAY) {
            pthread_cond_wait(&q->cond, &q->mutex);
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout / 1000;
            ts.tv_nsec += (timeout % 1000) * 1000000;
            result = pthread_cond_timedwait(&q->cond, &q->mutex, &ts);
        }
    }
    if (q->items.size() < q->max_items) {
        std::vector<uint8_t> buf((const uint8_t *)item, (const uint8_t *)item + q->item_size);
        q->items.push(std::move(buf));
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
        return pdTRUE;
    }
    pthread_mutex_unlock(&q->mutex);
    return pdFALSE;
}

BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t timeout) {
    if (!queue || !item) return pdFALSE;
    auto *q = static_cast<QueueInfo *>(queue);
    pthread_mutex_lock(&q->mutex);
    int result = 0;
    while (q->items.empty() && result == 0) {
        if (timeout == portMAX_DELAY) {
            pthread_cond_wait(&q->cond, &q->mutex);
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout / 1000;
            ts.tv_nsec += (timeout % 1000) * 1000000;
            result = pthread_cond_timedwait(&q->cond, &q->mutex, &ts);
        }
    }
    if (!q->items.empty()) {
        memcpy(item, q->items.front().data(), q->item_size);
        q->items.pop();
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
        return pdTRUE;
    }
    pthread_mutex_unlock(&q->mutex);
    return pdFALSE;
}

void vQueueDelete(QueueHandle_t queue) {
    if (!queue) return;
    auto *q = static_cast<QueueInfo *>(queue);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
    delete q;
}

// ═══════════════════════════════════════════════════════════════════════════════
// NVS Flash Mock
// ═══════════════════════════════════════════════════════════════════════════════

struct NVSBlob {
    std::vector<uint8_t> data;
};

struct NVSNamespace {
    std::map<std::string, NVSBlob> keys;
};

static std::mutex s_nvs_mutex;
static std::map<std::string, NVSNamespace> s_nvs_storage;
static uint32_t s_nvs_next_handle = 1;
static std::map<uint32_t, std::string> s_nvs_handles;
static bool s_nvs_initialized = false;

esp_err_t nvs_flash_init(void) {
    s_nvs_initialized = true;
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    s_nvs_storage.clear();
    s_nvs_handles.clear();
    return ESP_OK;
}

esp_err_t nvs_open(const char *ns, nvs_open_mode_t mode, nvs_handle_t *out_handle) {
    (void)mode;
    if (!ns || !out_handle) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    if (s_nvs_storage.find(ns) == s_nvs_storage.end()) {
        s_nvs_storage[ns] = NVSNamespace{};
    }
    uint32_t h = s_nvs_next_handle++;
    s_nvs_handles[h] = ns;
    *out_handle = h;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    s_nvs_handles.erase(handle);
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    s_nvs_storage[it->second].keys.erase(key);
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    s_nvs_storage[it->second].keys.clear();
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    (void)handle;
    return ESP_OK;
}

template<typename T>
static esp_err_t nvs_get_val(nvs_handle_t handle, const char *key, T *out) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    auto kit = s_nvs_storage[it->second].keys.find(key);
    if (kit == s_nvs_storage[it->second].keys.end()) return ESP_ERR_NVS_NOT_FOUND;
    if (kit->second.data.size() != sizeof(T)) return ESP_ERR_NVS_NOT_FOUND;
    memcpy(out, kit->second.data.data(), sizeof(T));
    return ESP_OK;
}

template<typename T>
static esp_err_t nvs_set_val(nvs_handle_t handle, const char *key, T val) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    NVSBlob blob;
    blob.data.resize(sizeof(T));
    memcpy(blob.data.data(), &val, sizeof(T));
    s_nvs_storage[it->second].keys[key] = blob;
    return ESP_OK;
}

esp_err_t nvs_get_i8(nvs_handle_t h, const char *k, int8_t *v)  { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_u8(nvs_handle_t h, const char *k, uint8_t *v)  { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_i16(nvs_handle_t h, const char *k, int16_t *v) { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_u16(nvs_handle_t h, const char *k, uint16_t *v) { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_i32(nvs_handle_t h, const char *k, int32_t *v) { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_u32(nvs_handle_t h, const char *k, uint32_t *v) { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_i64(nvs_handle_t h, const char *k, int64_t *v) { return nvs_get_val(h, k, v); }
esp_err_t nvs_get_u64(nvs_handle_t h, const char *k, uint64_t *v) { return nvs_get_val(h, k, v); }

esp_err_t nvs_set_i8(nvs_handle_t h, const char *k, int8_t v)  { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_u8(nvs_handle_t h, const char *k, uint8_t v)  { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_i16(nvs_handle_t h, const char *k, int16_t v) { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_u16(nvs_handle_t h, const char *k, uint16_t v) { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_i32(nvs_handle_t h, const char *k, int32_t v) { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_u32(nvs_handle_t h, const char *k, uint32_t v) { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_i64(nvs_handle_t h, const char *k, int64_t v) { return nvs_set_val(h, k, v); }
esp_err_t nvs_set_u64(nvs_handle_t h, const char *k, uint64_t v) { return nvs_set_val(h, k, v); }

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out, size_t *len) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    auto kit = s_nvs_storage[it->second].keys.find(key);
    if (kit == s_nvs_storage[it->second].keys.end()) return ESP_ERR_NVS_NOT_FOUND;
    const auto &data = kit->second.data;
    size_t copy = *len < data.size() ? *len : data.size();
    memcpy(out, data.data(), copy);
    *len = data.size();
    return copy <= data.size() ? ESP_OK : ESP_ERR_INVALID_ARG;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *v) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    NVSBlob blob;
    size_t len = strlen(v) + 1;
    blob.data.assign(v, v + len);
    s_nvs_storage[it->second].keys[key] = blob;
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out, size_t *len) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    auto kit = s_nvs_storage[it->second].keys.find(key);
    if (kit == s_nvs_storage[it->second].keys.end()) return ESP_ERR_NVS_NOT_FOUND;
    const auto &data = kit->second.data;
    size_t copy = *len < data.size() ? *len : data.size();
    memcpy(out, data.data(), copy);
    *len = data.size();
    return copy <= data.size() ? ESP_OK : ESP_ERR_INVALID_ARG;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *v, size_t len) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_nvs_handles.find(handle);
    if (it == s_nvs_handles.end()) return ESP_ERR_INVALID_ARG;
    NVSBlob blob;
    blob.data.assign((const uint8_t *)v, (const uint8_t *)v + len);
    s_nvs_storage[it->second].keys[key] = blob;
    return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════════
// WiFi Stubs
// ═══════════════════════════════════════════════════════════════════════════════

int esp_wifi_init(const void *cfg) { (void)cfg; return ESP_OK; }
int esp_wifi_start(void) { return ESP_OK; }
int esp_wifi_stop(void) { return ESP_OK; }
int esp_wifi_disconnect(void) { return ESP_OK; }
int esp_wifi_scan_start(const void *cfg, bool block) { (void)cfg; (void)block; return ESP_OK; }
int esp_wifi_scan_stop(void) { return ESP_OK; }
int esp_wifi_scan_get_ap_num(uint16_t *num) { if (num) *num = 0; return ESP_OK; }
int esp_wifi_scan_get_ap_records(uint16_t *num, wifi_ap_record_t *rec) { (void)num; (void)rec; return ESP_OK; }
int esp_wifi_set_promiscuous(bool enable) { (void)enable; return ESP_OK; }
int esp_wifi_set_promiscuous_filter(const wifi_promiscuous_filter_t *filter) { (void)filter; return ESP_OK; }
int esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb) { (void)cb; return ESP_OK; }
int esp_wifi_set_channel(uint8_t primary, int secondary) { (void)primary; (void)secondary; return ESP_OK; }
int esp_wifi_get_channel(uint8_t *primary, int *secondary) { if (primary) *primary = 6; if (secondary) *secondary = 0; return ESP_OK; }

// ═══════════════════════════════════════════════════════════════════════════════
// GPIO Stubs
// ═══════════════════════════════════════════════════════════════════════════════

static std::mutex s_gpio_mutex;
static std::map<gpio_num_t, bool> s_gpio_levels;

static constexpr gpio_num_t HLE_GPIO_BTN_LEFT_PIN = 5;
static constexpr gpio_num_t HLE_GPIO_BTN_BACK_PIN = 7;
static constexpr gpio_num_t HLE_GPIO_BTN_UP_PIN = 15;
static constexpr gpio_num_t HLE_GPIO_BTN_DOWN_PIN = 6;
static constexpr gpio_num_t HLE_GPIO_BTN_OK_PIN = 4;
static constexpr gpio_num_t HLE_GPIO_BTN_RIGHT_PIN = 16;

esp_err_t gpio_config(const gpio_config_t *cfg) {
    if (!cfg) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_gpio_mutex);
    for (gpio_num_t pin = 0; pin < 64; ++pin) {
        if ((cfg->pin_bit_mask & (1ULL << pin)) == 0) continue;
        if ((cfg->mode == GPIO_MODE_INPUT || cfg->mode == GPIO_MODE_INPUT_OUTPUT) && cfg->pull_up_en) {
            s_gpio_levels[pin] = true;
        } else if ((cfg->mode == GPIO_MODE_INPUT || cfg->mode == GPIO_MODE_INPUT_OUTPUT) && cfg->pull_down_en) {
            s_gpio_levels[pin] = false;
        }
    }
    return ESP_OK;
}
esp_err_t gpio_set_level(gpio_num_t pin, uint32_t level) {
    std::lock_guard<std::mutex> lock(s_gpio_mutex);
    s_gpio_levels[pin] = (level != 0);
    return ESP_OK;
}
int gpio_get_level(gpio_num_t pin) {
    std::lock_guard<std::mutex> lock(s_gpio_mutex);
    auto it = s_gpio_levels.find(pin);
    return (it != s_gpio_levels.end() && it->second) ? 1 : 0;
}
esp_err_t gpio_install_isr_service(int flags) { (void)flags; return ESP_OK; }
esp_err_t gpio_isr_handler_add(gpio_num_t pin, void (*handler)(void *), void *arg) {
    (void)pin; (void)handler; (void)arg; return ESP_OK;
}
void gpio_reset_pin(gpio_num_t pin) { (void)pin; }

extern "C" void hle_set_button_mask(unsigned char mask) {
    std::lock_guard<std::mutex> lock(s_gpio_mutex);

    s_gpio_levels[HLE_GPIO_BTN_UP_PIN] = (mask & (1u << 0)) == 0;
    s_gpio_levels[HLE_GPIO_BTN_DOWN_PIN] = (mask & (1u << 1)) == 0;
    s_gpio_levels[HLE_GPIO_BTN_LEFT_PIN] = (mask & (1u << 2)) == 0;
    s_gpio_levels[HLE_GPIO_BTN_RIGHT_PIN] = (mask & (1u << 3)) == 0;
    s_gpio_levels[HLE_GPIO_BTN_OK_PIN] = (mask & (1u << 4)) == 0;
    s_gpio_levels[HLE_GPIO_BTN_BACK_PIN] = (mask & (1u << 5)) == 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// I2C Stubs
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t i2c_param_config(i2c_port_t port, const i2c_config_t *cfg) { (void)port; (void)cfg; return ESP_OK; }
esp_err_t i2c_driver_install(i2c_port_t port, int mode, int rx_buf, int tx_buf, int flags) {
    (void)port; (void)mode; (void)rx_buf; (void)tx_buf; (void)flags; return ESP_OK;
}
i2c_cmd_handle_t i2c_cmd_link_create(void) { return 0; }
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd) { (void)cmd; return ESP_OK; }
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en) {
    (void)cmd; (void)data; (void)ack_en; return ESP_OK;
}
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd, uint8_t *data, int ack) {
    (void)cmd; (void)ack; if (data) *data = 0; return ESP_OK;
}
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd) { (void)cmd; return ESP_OK; }
esp_err_t i2c_master_cmd_begin(i2c_port_t port, i2c_cmd_handle_t cmd, int timeout_ms) {
    (void)port; (void)cmd; (void)timeout_ms; return ESP_OK;
}
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd) { (void)cmd; }

// ═══════════════════════════════════════════════════════════════════════════════
// SPI Stubs
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t spi_bus_initialize(spi_host_device_t host, const spi_bus_config_t *bus, int dma) {
    (void)host; (void)bus; (void)dma; return ESP_OK;
}
esp_err_t spi_bus_add_device(spi_host_device_t host, const spi_device_interface_config_t *dev, spi_device_handle_t *handle) {
    (void)host; (void)dev;
    if (handle) *handle = (void *)(uintptr_t)1; // non-NULL to pass checks
    return ESP_OK;
}
esp_err_t spi_device_transmit(spi_device_handle_t handle, const void *trans) { (void)handle; (void)trans; return ESP_OK; }
esp_err_t spi_device_polling_transmit(spi_device_handle_t handle, const void *trans) {
    (void)handle; (void)trans; return ESP_OK;
}
esp_err_t spi_bus_remove_device(spi_device_handle_t handle) { (void)handle; return ESP_OK; }

// SPI Bridge PHY stubs — placeholder only.
// These stubs satisfy linker dependencies for P4 spi_bridge.c, but they do not
// forward frames into the in-memory HLE bridge channel. Callers using
// spi_bridge_send_command() will receive ESP_ERR_NOT_SUPPORTED until a proper
// transport shim is implemented.
extern "C" {
esp_err_t spi_bridge_phy_init(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t spi_bridge_phy_transmit(const uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    (void)tx_data;
    if (rx_data) memset(rx_data, 0, len);
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t spi_bridge_phy_wait_irq(uint32_t timeout_ms) {
    (void)timeout_ms;
    return ESP_ERR_NOT_SUPPORTED;
}
}

// ═══════════════════════════════════════════════════════════════════════════════
// SPI Bridge Channel Integration
// ═══════════════════════════════════════════════════════════════════════════════

static hle::SPIBridgeChannel *s_bridge_channel = nullptr;

void hle_set_bridge_channel(hle::SPIBridgeChannel *ch) { s_bridge_channel = ch; }

static const size_t B_FRAME = 260;

// SPI slave stubs — wired to in-memory channel
esp_err_t spi_slave_initialize(int host, const spi_bus_config_t *bus,
                                const spi_slave_interface_config_t *cfg, int dma) {
    (void)host; (void)bus; (void)cfg; (void)dma; return ESP_OK;
}

esp_err_t spi_slave_transmit(int host, spi_slave_transaction_t *trans, int timeout) {
    (void)host; (void)timeout;
    if (!s_bridge_channel) return ESP_FAIL;

    if (trans->tx_buffer == nullptr && trans->rx_buffer != nullptr) {
        // Slave is waiting for a command from master
        uint8_t cmd_id, payload[256], len;
        if (!s_bridge_channel->slave_wait_command(cmd_id, payload, len)) return ESP_FAIL;

        // Copy command frame into rx_buffer
        uint8_t *rx = static_cast<uint8_t *>(trans->rx_buffer);
        memset(rx, 0, B_FRAME);
        rx[0] = 0xAA;  // sync
        rx[1] = 0x01;  // CMD type
        rx[2] = cmd_id;
        rx[3] = len;
        if (len > 0) memcpy(rx + 4, payload, len);
    } else if (trans->tx_buffer != nullptr && trans->rx_buffer == nullptr) {
        // Slave is sending response back to master
        // The tx_buffer contains the response frame [sync, type, id, len, status, payload...]
        const uint8_t *tx = static_cast<const uint8_t *>(trans->tx_buffer);
        uint8_t sync = tx[0], type = tx[1], cmd_id = tx[2], plen = tx[3];
        if (sync != 0xAA) return ESP_FAIL;
        if (type == 0x02) {
            // Response: payload starts at byte 5 (after header + status)
            uint8_t status = (plen > 0) ? tx[4] : 0;
            uint8_t data_len = (plen > 0) ? plen - 1 : 0;
            s_bridge_channel->slave_send_response(cmd_id, status, tx + 5, data_len);
        } else if (type == 0x03) {
            // Stream
            uint8_t data_len = (plen > 0) ? plen : 0;
            s_bridge_channel->slave_send_stream(cmd_id, tx + 4, data_len);
        }
        s_bridge_channel->slave_notify_irq();
    }

    return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════════
// LED Strip Stubs
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t led_strip_new_rmt_device(const led_strip_config_t *strip_cfg,
                                    const led_strip_rmt_config_t *rmt_cfg,
                                    led_strip_handle_t *handle) {
    (void)strip_cfg; (void)rmt_cfg;
    if (handle) *handle = (void *)(uintptr_t)1;
    return ESP_OK;
}
esp_err_t led_strip_set_pixel(led_strip_handle_t handle, uint32_t index, uint32_t r, uint32_t g, uint32_t b) {
    (void)handle; (void)index; (void)r; (void)g; (void)b; return ESP_OK;
}
esp_err_t led_strip_refresh(led_strip_handle_t handle) { (void)handle; return ESP_OK; }
esp_err_t led_strip_clear(led_strip_handle_t handle) { (void)handle; return ESP_OK; }
esp_err_t led_strip_del(led_strip_handle_t handle) { (void)handle; return ESP_OK; }

// ═══════════════════════════════════════════════════════════════════════════════
// RMT Stubs
// ═══════════════════════════════════════════════════════════════════════════════

esp_err_t rmt_config(const rmt_config_t *cfg) { (void)cfg; return ESP_OK; }
esp_err_t rmt_driver_install(int channel, int rx_buf_size, int flags) {
    (void)channel; (void)rx_buf_size; (void)flags; return ESP_OK;
}
esp_err_t rmt_write_items(int channel, const void *data, int num_items, bool wait_done) {
    (void)channel; (void)data; (void)num_items; (void)wait_done; return ESP_OK;
}
esp_err_t rmt_driver_uninstall(int channel) { (void)channel; return ESP_OK; }

// SD card stubs
esp_err_t sdmmc_host_init(void) { return ESP_OK; }
esp_err_t sdmmc_host_init_slot(int slot, const void *config) { (void)slot; (void)config; return ESP_OK; }
void sdmmc_card_print_info(FILE *f, const sdmmc_card_t *card) {
    (void)f; (void)card; fprintf(stderr, "I [SDMMC] Mock SD card\n");
}
void sdmmc_host_deinit(void) {}

// ═══════════════════════════════════════════════════════════════════════════════
// ESP LCD Panel Stubs (wired to HLE Display)
// ═══════════════════════════════════════════════════════════════════════════════

typedef void (*hle_color_trans_done_t)(void *, void *, void *);
struct HLEPanel {
    hle_color_trans_done_t on_color_trans_done;
    void *ctx;
};

esp_err_t esp_lcd_new_panel_io_spi(int bus, const esp_lcd_panel_io_spi_config_t *cfg,
                                    esp_lcd_panel_io_handle_t *out_io) {
    (void)bus; (void)cfg;
    auto *io = new HLEPanel{};
    if (out_io) *out_io = io;
    return ESP_OK;
}

esp_err_t esp_lcd_new_panel_st7789(esp_lcd_panel_io_handle_t io,
                                    const esp_lcd_panel_dev_config_t *cfg,
                                    esp_lcd_panel_handle_t *out_panel) {
    (void)io; (void)cfg;
    if (out_panel) *out_panel = io;
    return ESP_OK;
}

esp_err_t esp_lcd_panel_reset(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }
esp_err_t esp_lcd_panel_init(esp_lcd_panel_handle_t panel) { (void)panel; return ESP_OK; }
esp_err_t esp_lcd_panel_invert_color(esp_lcd_panel_handle_t panel, bool invert) { (void)panel; (void)invert; return ESP_OK; }
esp_err_t esp_lcd_panel_disp_on_off(esp_lcd_panel_handle_t panel, bool on) { (void)panel; (void)on; return ESP_OK; }
esp_err_t esp_lcd_panel_mirror(esp_lcd_panel_handle_t panel, bool mx, bool my) { (void)panel; (void)mx; (void)my; return ESP_OK; }
esp_err_t esp_lcd_panel_swap_xy(esp_lcd_panel_handle_t panel, bool swap) { (void)panel; (void)swap; return ESP_OK; }
esp_err_t esp_lcd_panel_set_gap(esp_lcd_panel_handle_t panel, int x, int y) { (void)panel; (void)x; (void)y; return ESP_OK; }

esp_err_t esp_lcd_panel_draw_bitmap(esp_lcd_panel_handle_t panel, int x0, int y0, int x1, int y1,
                                     const void *data) {
    (void)panel;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > hle::LCD_H_RES) x1 = hle::LCD_H_RES;
    if (y1 > hle::LCD_V_RES) y1 = hle::LCD_V_RES;
    hle::Display::instance().draw_bitmap(x0, y0, x1, y1, static_cast<const uint16_t *>(data));

    // Notify LVGL flush ready
    auto *io = static_cast<HLEPanel *>(panel);
    if (io && io->on_color_trans_done) {
        io->on_color_trans_done(io, nullptr, io->ctx);
    }
    return ESP_OK;
}

esp_err_t esp_lcd_panel_io_register_event_callbacks(esp_lcd_panel_io_handle_t io,
    const esp_lcd_panel_io_callbacks_t *cbs, void *ctx) {
    auto *p = static_cast<HLEPanel *>(io);
    if (p && cbs) {
        p->on_color_trans_done = (hle_color_trans_done_t)cbs->on_color_trans_done;
        p->ctx = ctx;
    }
    return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════════
// VFS / Storage stubs (mount to host tmpdir)
// ═══════════════════════════════════════════════════════════════════════════════

#include <sys/stat.h>
#include <sys/types.h>

static std::string s_base_path = "/tmp/hle_storage";

void hle_set_storage_path(const char *path) { s_base_path = path; }

static std::string hle_translate_path(const char *path) {
    if (!path || path[0] == '\0') return {};

    const std::string src(path);
    if (src.rfind("/sdcard", 0) == 0) {
        return s_base_path + src;
    }

    if (src.rfind("/assets", 0) == 0) {
#ifdef HLE_ASSETS_DIR
        return std::string(HLE_ASSETS_DIR) + src.substr(sizeof("/assets") - 1);
#else
        return s_base_path + src;
#endif
    }

    return src;
}

extern "C" FILE *__real_fopen(const char *path, const char *mode);
extern "C" int __real_stat(const char *path, struct stat *st);
extern "C" DIR *__real_opendir(const char *path);
extern "C" int __real_mkdir(const char *path, mode_t mode);
extern "C" int __real_access(const char *path, int mode);
extern "C" int __real_remove(const char *path);
extern "C" int __real_rename(const char *old_path, const char *new_path);
extern "C" int __real_unlink(const char *path);

extern "C" FILE *__wrap_fopen(const char *path, const char *mode) {
    const std::string translated = hle_translate_path(path);
    return __real_fopen(translated.c_str(), mode);
}

extern "C" int __wrap_stat(const char *path, struct stat *st) {
    const std::string translated = hle_translate_path(path);
    return __real_stat(translated.c_str(), st);
}

extern "C" DIR *__wrap_opendir(const char *path) {
    const std::string translated = hle_translate_path(path);
    return __real_opendir(translated.c_str());
}

extern "C" int __wrap_mkdir(const char *path, mode_t mode) {
    const std::string translated = hle_translate_path(path);
    return __real_mkdir(translated.c_str(), mode);
}

extern "C" int __wrap_access(const char *path, int mode) {
    const std::string translated = hle_translate_path(path);
    return __real_access(translated.c_str(), mode);
}

extern "C" int __wrap_remove(const char *path) {
    const std::string translated = hle_translate_path(path);
    return __real_remove(translated.c_str());
}

extern "C" int __wrap_rename(const char *old_path, const char *new_path) {
    const std::string translated_old = hle_translate_path(old_path);
    const std::string translated_new = hle_translate_path(new_path);
    return __real_rename(translated_old.c_str(), translated_new.c_str());
}

extern "C" int __wrap_unlink(const char *path) {
    const std::string translated = hle_translate_path(path);
    return __real_unlink(translated.c_str());
}

extern "C" {

esp_err_t esp_vfs_fat_sdmmc_mount(const char *base_path, const sdmmc_host_t *host, const void *slot,
                                   const void *mount_conf, sdmmc_card_t **out_card) {
    (void)host; (void)slot; (void)mount_conf;
    std::string dir = s_base_path + (base_path ? base_path : "/sdcard");
    mkdir(dir.c_str(), 0755);
    fprintf(stderr, "I [VFS] Mounted HLE storage at %s\n", dir.c_str());
    if (out_card) *out_card = (sdmmc_card_t *)0x1;
    return ESP_OK;
}

esp_err_t esp_vfs_fat_sdmmc_unmount(void) { return ESP_OK; }
esp_err_t esp_vfs_fat_sdcard_unmount(const char *base_path, sdmmc_card_t *card) {
    (void)base_path; (void)card; return ESP_OK;
}

esp_err_t esp_vfs_fat_spiflash_mount(const char *base, const char *part_label, const void *mount, void *wl_handle) {
    (void)base; (void)part_label; (void)mount; (void)wl_handle;
    std::string dir = s_base_path + "/assets";
    mkdir(dir.c_str(), 0755);
    return ESP_OK;
}

esp_err_t esp_vfs_fat_spiflash_unmount(const char *base, void *wl_handle) {
    (void)base; (void)wl_handle; return ESP_OK;
}

} // extern "C"

// LittleFS stubs
esp_err_t esp_vfs_littlefs_register(const esp_vfs_littlefs_conf_t *conf) {
    if (conf && conf->base_path) {
        std::string dir = s_base_path + conf->base_path;
        mkdir(dir.c_str(), 0755);
        fprintf(stderr, "I [LFS] Registered HLE LittleFS at %s\n", conf->base_path);
    }
    return ESP_OK;
}
esp_err_t esp_vfs_littlefs_unregister(const char *label) { (void)label; return ESP_OK; }

// Partition stubs
esp_partition_t *esp_partition_find_first(int type, int subtype, const char *label) {
    (void)type; (void)subtype; (void)label;
    static esp_partition_t p = {0, 8*1024*1024, "storage"};
    return &p;
}
const esp_partition_t *esp_ota_get_running_partition(void) { return nullptr; }
void esp_partition_get_sha256(const esp_partition_t *p, uint8_t *sha) { (void)p; memset(sha, 0, 32); }
