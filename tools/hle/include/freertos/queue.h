#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

QueueHandle_t xQueueCreate(UBaseType_t queue_len, UBaseType_t item_size);
QueueHandle_t xQueueCreateStatic(UBaseType_t queue_len, UBaseType_t item_size,
                                  uint8_t *storage, StaticQueue_t *queue_buf);
BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t timeout);
BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t timeout);
void vQueueDelete(QueueHandle_t queue);

#ifdef __cplusplus
}
#endif
