#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int wifi_interface_t;
#define WIFI_IF_STA 0
#define WIFI_IF_AP  1

typedef struct {
    uint8_t bssid[6];
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
} wifi_ap_record_t;

typedef void (*wifi_promiscuous_cb_t)(void *buf, int type);
typedef struct { int flags; } wifi_promiscuous_filter_t;

int esp_wifi_init(const void *cfg);
int esp_wifi_start(void);
int esp_wifi_stop(void);
int esp_wifi_disconnect(void);
int esp_wifi_scan_start(const void *cfg, bool block);
int esp_wifi_scan_stop(void);
int esp_wifi_scan_get_ap_num(uint16_t *num);
int esp_wifi_scan_get_ap_records(uint16_t *num, wifi_ap_record_t *rec);
int esp_wifi_set_promiscuous(bool enable);
int esp_wifi_set_promiscuous_filter(const wifi_promiscuous_filter_t *filter);
int esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb);
int esp_wifi_set_channel(uint8_t primary, int secondary);
int esp_wifi_get_channel(uint8_t *primary, int *secondary);

#define WIFI_INIT_CONFIG_DEFAULT() ((void*)0)
#define WIFI_AUTH_OPEN      0
#define WIFI_AUTH_WPA2_PSK  3
#define WIFI_PROTOCOL_11B   1
#define WIFI_PROTOCOL_11G   2

#ifdef __cplusplus
}
#endif
