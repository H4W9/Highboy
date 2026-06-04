#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int ledc_timer_t;
typedef int ledc_channel_t;
typedef int ledc_mode_t;
#define LEDC_LOW_SPEED_MODE 0
#define LEDC_TIMER_0        0
#define LEDC_CHANNEL_0      0
#define LEDC_TIMER_10_BIT   10

typedef struct { int speed_mode; int timer_num; int freq_hz; int duty_resolution; } ledc_timer_config_t;
typedef struct { int gpio_num; int speed_mode; int channel; int timer_sel; int duty; } ledc_channel_config_t;

#ifdef __cplusplus
}
#endif
