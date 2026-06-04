extern "C" {
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
}

#include <lvgl.h>
#include <cstdint>

#define KEY_UP     (1 << 0)
#define KEY_DOWN   (1 << 1)
#define KEY_LEFT   (1 << 2)
#define KEY_RIGHT  (1 << 3)
#define KEY_OK     (1 << 4)
#define KEY_BACK   (1 << 5)

static const char *TAG = "LV_PORT_INDEV";

extern "C" {

lv_indev_t *indev_keypad = nullptr;
lv_group_t *main_group = nullptr;

void hle_lv_port_indev_init(void) {
    indev_keypad = lv_indev_create();
    lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);

    main_group = lv_group_create();
    lv_group_set_default(main_group);
    lv_indev_set_group(indev_keypad, main_group);

    ESP_LOGI(TAG, "HLE input device initialized");
}

} // extern "C"
