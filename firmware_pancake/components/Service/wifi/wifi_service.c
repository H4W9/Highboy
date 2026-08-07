// Copyright (c) 2025 HIGH CODE LLC
//
// TentacleOS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TentacleOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TentacleOS. If not, see <https://www.gnu.org/licenses/>.

#include "wifi_service.h"

#include <string.h>

#include "esp_log.h"

#include "spi_bridge.h"

static const char *TAG = "WIFI_SERVICE_P4";

#define SSID_MAX_LEN 33

static wifi_ap_record_t s_cached_record;
static char s_connected_ssid[SSID_MAX_LEN] = {0};

void wifi_service_init(void) {
  spi_bridge_send_command(
      SPI_ID_WIFI_START, NULL, 0, NULL, NULL, spi_bridge_get_timeout(SPI_ID_WIFI_START));
}

void wifi_service_deinit(void) {
  spi_bridge_send_command(
      SPI_ID_WIFI_STOP, NULL, 0, NULL, NULL, spi_bridge_get_timeout(SPI_ID_WIFI_STOP));
}

void wifi_service_start(void) {
  spi_bridge_send_command(
      SPI_ID_WIFI_START, NULL, 0, NULL, NULL, spi_bridge_get_timeout(SPI_ID_WIFI_START));
}

void wifi_service_stop(void) {
  spi_bridge_send_command(
      SPI_ID_WIFI_STOP, NULL, 0, NULL, NULL, spi_bridge_get_timeout(SPI_ID_WIFI_STOP));
}

void wifi_service_scan(void) {
  spi_bridge_send_command(
      SPI_ID_WIFI_SCAN, NULL, 0, NULL, NULL, spi_bridge_get_timeout(SPI_ID_WIFI_SCAN));
}

uint16_t wifi_service_get_ap_count(void) {
  spi_header_t resp;
  uint8_t payload[2];
  uint16_t magic_count = SPI_DATA_INDEX_COUNT;

  if (spi_bridge_send_command(SPI_ID_SYSTEM_DATA,
                              (uint8_t *)&magic_count,
                              2,
                              &resp,
                              payload,
                              spi_bridge_get_timeout(SPI_ID_SYSTEM_DATA)) == ESP_OK) {
    uint16_t count;
    memcpy(&count, payload, 2);
    return count;
  }
  return 0;
}

wifi_ap_record_t *wifi_service_get_ap_record(uint16_t index) {
  spi_header_t resp;

  if (spi_bridge_send_command(SPI_ID_SYSTEM_DATA,
                              (uint8_t *)&index,
                              2,
                              &resp,
                              (uint8_t *)&s_cached_record,
                              spi_bridge_get_timeout(SPI_ID_SYSTEM_DATA)) == ESP_OK) {
    return &s_cached_record;
  }
  return NULL;
}

esp_err_t wifi_service_connect_to_ap(const char *ssid, const char *password) {
  spi_wifi_connect_t cfg = {0};
  strncpy(cfg.ssid, ssid, sizeof(cfg.ssid));
  if (password != NULL) {
    strncpy(cfg.password, password, sizeof(cfg.password));
  }
  return spi_bridge_send_command(SPI_ID_WIFI_CONNECT,
                                 (uint8_t *)&cfg,
                                 sizeof(cfg),
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_CONNECT));
}

bool wifi_service_is_connected(void) {
  spi_header_t resp;
  spi_system_status_t sys = {0};

  if (spi_bridge_send_command(SPI_ID_SYSTEM_STATUS,
                              NULL,
                              0,
                              &resp,
                              (uint8_t *)&sys,
                              spi_bridge_get_timeout(SPI_ID_SYSTEM_STATUS)) == ESP_OK) {
    return sys.wifi_connected != 0;
  }
  return (wifi_service_get_connected_ssid() != NULL);
}

const char *wifi_service_get_connected_ssid(void) {
  spi_header_t resp;

  if (spi_bridge_send_command(SPI_ID_WIFI_GET_STA_INFO,
                              NULL,
                              0,
                              &resp,
                              (uint8_t *)s_connected_ssid,
                              spi_bridge_get_timeout(SPI_ID_WIFI_GET_STA_INFO)) == ESP_OK) {
    s_connected_ssid[SSID_MAX_LEN - 1] = '\0';
    return s_connected_ssid;
  }
  return NULL;
}

bool wifi_service_is_active(void) {
  spi_header_t resp;
  spi_system_status_t sys = {0};

  if (spi_bridge_send_command(SPI_ID_SYSTEM_STATUS,
                              NULL,
                              0,
                              &resp,
                              (uint8_t *)&sys,
                              spi_bridge_get_timeout(SPI_ID_SYSTEM_STATUS)) == ESP_OK) {
    return sys.wifi_active != 0;
  }
  return false;
}

void wifi_service_change_to_hotspot(const char *new_ssid) {
  wifi_service_set_ap_ssid(new_ssid);
}

esp_err_t wifi_service_save_ap_config(
    const char *ssid, const char *password, uint8_t max_conn, const char *ip_addr, bool enabled) {
  spi_wifi_ap_config_t cfg = {0};

  if (ssid == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  strncpy(cfg.ssid, ssid, sizeof(cfg.ssid));
  if (password != NULL) {
    strncpy(cfg.password, password, sizeof(cfg.password));
  }
  cfg.max_conn = max_conn;
  if (ip_addr != NULL) {
    strncpy(cfg.ip_addr, ip_addr, sizeof(cfg.ip_addr));
  }
  cfg.enabled = enabled ? 1 : 0;

  return spi_bridge_send_command(SPI_ID_WIFI_SAVE_AP_CONFIG,
                                 (uint8_t *)&cfg,
                                 sizeof(cfg),
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_SAVE_AP_CONFIG));
}

esp_err_t wifi_service_set_enabled(bool enabled) {
  uint8_t payload = enabled ? 1 : 0;
  return spi_bridge_send_command(SPI_ID_WIFI_SET_ENABLED,
                                 &payload,
                                 1,
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_SET_ENABLED));
}

esp_err_t wifi_service_set_ap_ssid(const char *ssid) {
  return spi_bridge_send_command(SPI_ID_WIFI_SET_AP,
                                 (uint8_t *)ssid,
                                 strlen(ssid),
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_SET_AP));
}

esp_err_t wifi_service_set_ap_password(const char *password) {
  if (password == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  return spi_bridge_send_command(SPI_ID_WIFI_SET_AP_PASSWORD,
                                 (uint8_t *)password,
                                 strlen(password),
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_SET_AP_PASSWORD));
}

esp_err_t wifi_service_set_ap_max_conn(uint8_t max_conn) {
  uint8_t payload = max_conn;
  return spi_bridge_send_command(SPI_ID_WIFI_SET_AP_MAX_CONN,
                                 &payload,
                                 1,
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_SET_AP_MAX_CONN));
}

esp_err_t wifi_service_set_ap_ip(const char *ip_addr) {
  if (ip_addr == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  return spi_bridge_send_command(SPI_ID_WIFI_SET_AP_IP,
                                 (uint8_t *)ip_addr,
                                 strlen(ip_addr),
                                 NULL,
                                 NULL,
                                 spi_bridge_get_timeout(SPI_ID_WIFI_SET_AP_IP));
}

void wifi_service_promiscuous_start(wifi_promiscuous_cb_t cb, wifi_promiscuous_filter_t *filter) {
  (void)cb;
  (void)filter;
  esp_err_t err = spi_bridge_send_command(SPI_ID_WIFI_PROMISC_START,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          spi_bridge_get_timeout(SPI_ID_WIFI_PROMISC_START));
  if (err == ESP_ERR_NOT_SUPPORTED) {
    ESP_LOGW(TAG, "Promiscuous start not supported over SPI");
  }
}

void wifi_service_promiscuous_stop(void) {
  esp_err_t err = spi_bridge_send_command(SPI_ID_WIFI_PROMISC_STOP,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          spi_bridge_get_timeout(SPI_ID_WIFI_PROMISC_STOP));
  if (err == ESP_ERR_NOT_SUPPORTED) {
    ESP_LOGW(TAG, "Promiscuous stop not supported over SPI");
  }
}

void wifi_service_start_channel_hopping(void) {
  spi_bridge_send_command(SPI_ID_WIFI_CH_HOP_START,
                          NULL,
                          0,
                          NULL,
                          NULL,
                          spi_bridge_get_timeout(SPI_ID_WIFI_CH_HOP_START));
}

void wifi_service_stop_channel_hopping(void) {
  spi_bridge_send_command(SPI_ID_WIFI_CH_HOP_STOP,
                          NULL,
                          0,
                          NULL,
                          NULL,
                          spi_bridge_get_timeout(SPI_ID_WIFI_CH_HOP_STOP));
}
