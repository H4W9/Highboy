// Copyright (c) 2025 HIGH CODE LLC
//
// TentacleOS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TentacleOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TentacleOS. If not, see <https://www.gnu.org/licenses/>.

#include "sys_monitor.h"

#include <string.h>

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "kernel.h"
#include "ui_manager.h"

static const char *TAG = "SYS_MONITOR";

#define MONITOR_INTERVAL_MS      2000
#define MONITOR_STACK_SIZE       4096
#define MONITOR_PRIORITY         1
#define MONITOR_CORE             0
#define CRITICAL_STACK_THRESHOLD 256
#define ALERT_MSG_SIZE           128

typedef struct {
  bool is_verbose;
} sys_monitor_params_t;

static void check_task_stacks(TaskStatus_t *tasks, uint32_t count) {
  // Log-only. The previous behavior deleted any task whose stack high-water
  // mark dipped below the threshold (and restarted the UI). On the single-core
  // C5 that randomly kills system tasks (esp_timer, wifi, ...) and causes
  // freezes / black screens. FreeRTOS's own stack-overflow hook (kernel.c)
  // handles real overflows; here we only warn.
  for (uint32_t i = 0; i < count; i++) {
    uint32_t watermark = tasks[i].usStackHighWaterMark;
    if (watermark < CRITICAL_STACK_THRESHOLD) {
      ESP_LOGW(TAG,
               "Low stack in task [%s]: %lu words free",
               tasks[i].pcTaskName,
               (unsigned long)watermark);
    }
  }
}

static void sys_monitor_task(void *pvParameters) {
  sys_monitor_params_t *params = (sys_monitor_params_t *)pvParameters;
  bool is_verbose = params->is_verbose;
  vPortFree(params);

  ESP_LOGI(TAG, "System monitor started (verbose: %s)", is_verbose ? "enabled" : "disabled");

  (void)is_verbose;
  while (1) {
    // Always log RAM so memory trends are visible over the serial monitor.
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG,
             "RAM — Free: %lu, Internal: %lu, PSRAM: %lu",
             (unsigned long)free_heap,
             (unsigned long)internal_free,
             (unsigned long)spiram_free);

    uint32_t task_count = uxTaskGetNumberOfTasks();
    TaskStatus_t *task_array = pvPortMalloc(task_count * sizeof(TaskStatus_t));

    if (task_array != NULL) {
      task_count = uxTaskGetSystemState(task_array, task_count, NULL);
      check_task_stacks(task_array, task_count);
      vPortFree(task_array);
    } else {
      ESP_LOGE(TAG, "Failed to allocate task status array");
    }

    vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
  }
}

void sys_monitor_start(bool is_verbose) {
  sys_monitor_params_t *params = pvPortMalloc(sizeof(sys_monitor_params_t));
  if (params == NULL) {
    ESP_LOGE(TAG, "Failed to allocate monitor parameters");
    return;
  }

  params->is_verbose = is_verbose;

  xTaskCreatePinnedToCore(sys_monitor_task,
                          "SysMonitor",
                          MONITOR_STACK_SIZE,
                          (void *)params,
                          MONITOR_PRIORITY,
                          NULL,
                          MONITOR_CORE);
}
