#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int rmt_channel;
    int clk_div;
    int gpio;
    int mem_block_symbols;
    int flags;
} rmt_config_t;

#define RMT_CLK_SRC_DEFAULT 0
#define RMT_CHANNEL_0 0
#define RMT_RESOLUTION_HZ (10 * 1000 * 1000)

esp_err_t rmt_config(const rmt_config_t *cfg);
esp_err_t rmt_driver_install(int channel, int rx_buf_size, int flags);
esp_err_t rmt_write_items(int channel, const void *data, int num_items, bool wait_done);
esp_err_t rmt_driver_uninstall(int channel);

#ifdef __cplusplus
}
#endif
