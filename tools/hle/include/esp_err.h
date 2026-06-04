#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t esp_err_t;

#define ESP_OK          0
#define ESP_FAIL        (-1)
#define ESP_ERR_NO_MEM  0x101
#define ESP_ERR_INVALID_ARG  0x102
#define ESP_ERR_INVALID_SIZE 0x105
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_RESPONSE 0x104
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_NOT_FOUND 0x104
#define ESP_ERR_NOT_SUPPORTED 0x129
#define ESP_ERR_NVS_NO_FREE_PAGES     0x1100
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1101

// FreeRTOS compatibility for non-FreeRTOS code
#define portTICK_PERIOD_MS 1
#define portMAX_DELAY     ((uint32_t)0xFFFFFFFF)

#define ESP_OK_CHECK(x) do { esp_err_t _err = (x); if (_err != ESP_OK) return _err; } while(0)

#ifdef __cplusplus
}
#endif
