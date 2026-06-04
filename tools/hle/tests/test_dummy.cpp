#include <gtest/gtest.h>
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

TEST(HLEDummyTest, BuildSystemWorks) {
    EXPECT_EQ(ESP_OK, 0);
    EXPECT_NE(ESP_ERR_NO_MEM, ESP_OK);
}

TEST(HLEDummyTest, NVSFlashWorks) {
    ASSERT_EQ(nvs_flash_init(), ESP_OK);

    nvs_handle_t h;
    ASSERT_EQ(nvs_open("test", NVS_READWRITE, &h), ESP_OK);

    int32_t val = 42;
    ASSERT_EQ(nvs_set_i32(h, "answer", val), ESP_OK);
    ASSERT_EQ(nvs_commit(h), ESP_OK);

    int32_t read_val = 0;
    ASSERT_EQ(nvs_get_i32(h, "answer", &read_val), ESP_OK);
    EXPECT_EQ(read_val, 42);
    nvs_close(h);
}

TEST(HLEDummyTest, FreeRTOSMutex) {
    SemaphoreHandle_t mtx = xSemaphoreCreateMutex();
    ASSERT_NE(mtx, nullptr);
    EXPECT_EQ(xSemaphoreTake(mtx, 100), pdTRUE);
    EXPECT_EQ(xSemaphoreGive(mtx), pdTRUE);
    vSemaphoreDelete(mtx);
}

TEST(HLEDummyTest, FreeRTOSQueue) {
    QueueHandle_t q = xQueueCreate(4, sizeof(int32_t));
    ASSERT_NE(q, nullptr);

    int32_t send_val = 99;
    EXPECT_EQ(xQueueSend(q, &send_val, 0), pdTRUE);

    int32_t recv_val = 0;
    EXPECT_EQ(xQueueReceive(q, &recv_val, 100), pdTRUE);
    EXPECT_EQ(recv_val, 99);

    vQueueDelete(q);
}

TEST(HLEDummyTest, FreeRTOSTask) {
    static volatile bool ran = false;

    auto func = [](void *arg) {
        (void)arg;
        ran = true;
        vTaskDelay(10);
        vTaskDelete(nullptr);
    };

    TaskHandle_t handle;
    BaseType_t ret = xTaskCreatePinnedToCore(func, "dummy", 4096, nullptr, 1, &handle, 0);
    EXPECT_EQ(ret, pdPASS);

    // Give it time to run
    vTaskDelay(100);
    EXPECT_TRUE(ran);
}
