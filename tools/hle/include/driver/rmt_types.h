#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t level0 : 15;
    uint32_t duration0 : 15;
    uint32_t level1 : 1;
    uint32_t duration1 : 15;
    uint32_t duration : 16;
} rmt_item32_t;

typedef struct {
    uint32_t carrier_freq_hz;
    uint32_t duty_cycle;
    uint32_t carrier_level;
} rmt_carrier_config_t;

typedef struct {
    uint32_t carrier_en;
} rmt_transmit_config_t;

typedef struct {
    int rx;
    int tx;
} rmt_symbol_word_t;

#define RMT_MEM_BLOCK_BYTE_NUM 256

#ifdef __cplusplus
}
#endif
