#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define portTICK_PERIOD_MS 1
#define portSHORT          short

// ESP-IDF spinlock types (used by SPI bridge for stream queue)
typedef struct { int owner; int count; } portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED { -1, 0 }
static inline void portENTER_CRITICAL(portMUX_TYPE *mux) { (void)mux; }
static inline void portEXIT_CRITICAL(portMUX_TYPE *mux)  { (void)mux; }
static inline void portENTER_CRITICAL_ISR(portMUX_TYPE *mux) { (void)mux; }
static inline void portEXIT_CRITICAL_ISR(portMUX_TYPE *mux)  { (void)mux; }

#ifdef __cplusplus
}
#endif
