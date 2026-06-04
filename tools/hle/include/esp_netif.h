#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Stub for esp_netif
typedef int esp_netif_t;
typedef int esp_netif_config_t;
typedef int esp_event_base_t;
typedef int esp_event_handler_t;
typedef int esp_event_handler_instance_t;

#define ESP_NETIF_DEFAULT_WIFI_STA() (void*)0
#define ESP_NETIF_DEFAULT_WIFI_AP() (void*)0
#define WIFI_EVENT 0
#define IP_EVENT 0

#ifdef __cplusplus
}
#endif
