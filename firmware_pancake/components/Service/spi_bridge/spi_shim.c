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

// In-process radio shim for the single-chip Pancake build.
//
// Phase 2 milestone: WiFi active scan. The P4 ap_scanner client issues
// SPI_ID_WIFI_APP_SCAN_AP to run a scan, then SPI_ID_SYSTEM_DATA with a
// magic index to read the count and each wifi_ap_record_t. We service those
// directly with esp_wifi. Additional command IDs are lit up incrementally;
// anything unimplemented returns a fast benign error so the UI degrades
// gracefully (dummy) instead of blocking.

#include "spi_shim.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"

static const char *TAG = "SPI_SHIM";

#define SHIM_MAX_AP 64
#define SHIM_NVS_NS "prefs"

static bool s_wifi_ready = false;   // esp_wifi initialized/started
static bool s_wifi_enabled = false; // user-facing on/off (persisted)
static bool s_pref_loaded = false;
static wifi_ap_record_t s_aps[SHIM_MAX_AP];
static uint16_t s_ap_count = 0;

// ---- Persisted preferences (NVS) ------------------------------------------
static void load_prefs(void) {
  if (s_pref_loaded) {
    return;
  }
  s_pref_loaded = true;
  nvs_handle_t h;
  if (nvs_open(SHIM_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
    uint8_t v = 0;
    if (nvs_get_u8(h, "wifi_en", &v) == ESP_OK) {
      s_wifi_enabled = (v != 0);
    }
    nvs_close(h);
  }
  ESP_LOGI(TAG, "prefs loaded: wifi_enabled=%d", (int)s_wifi_enabled);
}

static void save_wifi_enabled(bool enabled) {
  nvs_handle_t h;
  if (nvs_open(SHIM_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
    nvs_set_u8(h, "wifi_en", enabled ? 1 : 0);
    nvs_commit(h);
    nvs_close(h);
  }
}

// Bring up esp_wifi in station mode once, tolerating already-initialized
// subsystems (netif / default event loop may be created elsewhere later).
static esp_err_t ensure_wifi(void) {
  if (s_wifi_ready) {
    return ESP_OK;
  }

  esp_err_t err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    return err;
  }

  err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    return err;
  }

  static esp_netif_t *s_sta_netif = NULL;
  if (s_sta_netif == NULL) {
    s_sta_netif = esp_netif_create_default_wifi_sta();
  }

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&cfg);
  if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
    ESP_LOGE(TAG, "esp_wifi_init: %s", esp_err_to_name(err));
    return err;
  }

  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);

  err = esp_wifi_start();
  if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_STOPPED) {
    ESP_LOGE(TAG, "esp_wifi_start: %s", esp_err_to_name(err));
    return err;
  }

  s_wifi_ready = true;
  ESP_LOGI(TAG, "WiFi backend ready (STA)");
  return ESP_OK;
}

static esp_err_t do_scan_ap(void) {
  if (ensure_wifi() != ESP_OK) {
    return ESP_FAIL;
  }

  wifi_scan_config_t scan = {
      .ssid = NULL,
      .bssid = NULL,
      .channel = 0,
      .show_hidden = true,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
  };

  esp_err_t err = esp_wifi_scan_start(&scan, true /* blocking */);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "scan_start: %s", esp_err_to_name(err));
    s_ap_count = 0;
    return err;
  }

  uint16_t num = SHIM_MAX_AP;
  err = esp_wifi_scan_get_ap_records(&num, s_aps);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "get_ap_records: %s", esp_err_to_name(err));
    s_ap_count = 0;
    return err;
  }

  s_ap_count = num;
  ESP_LOGI(TAG, "AP scan found %u networks", (unsigned)num);
  return ESP_OK;
}

// SPI_ID_SYSTEM_DATA: the client reads its result set here. The 2-byte payload
// is a magic index: SPI_DATA_INDEX_COUNT returns the count; otherwise it is a
// record index. Currently backed by the last AP scan.
static esp_err_t handle_system_data(const uint8_t *payload, uint8_t len, uint8_t *out_payload) {
  uint16_t index = 0;
  if (payload != NULL && len >= sizeof(uint16_t)) {
    memcpy(&index, payload, sizeof(uint16_t));
  }

  if (index == SPI_DATA_INDEX_COUNT) {
    if (out_payload != NULL) {
      memcpy(out_payload, &s_ap_count, sizeof(uint16_t));
    }
    return ESP_OK;
  }

  if (index < s_ap_count) {
    if (out_payload != NULL) {
      memcpy(out_payload, &s_aps[index], sizeof(wifi_ap_record_t));
    }
    return ESP_OK;
  }

  return ESP_FAIL;
}

static void set_wifi_enabled(bool enabled) {
  s_wifi_enabled = enabled;
  save_wifi_enabled(enabled);
  if (enabled) {
    ensure_wifi();
  }
}

esp_err_t spi_shim_dispatch(spi_id_t id,
                            const uint8_t *payload,
                            uint8_t len,
                            spi_header_t *out_header,
                            uint8_t *out_payload) {
  (void)out_header;
  load_prefs();

  switch (id) {
    // ---- WiFi scan ----
    case SPI_ID_WIFI_APP_SCAN_AP:
      return do_scan_ap();

    case SPI_ID_SYSTEM_DATA:
      return handle_system_data(payload, len, out_payload);

    // ---- WiFi enable/disable (persisted) ----
    case SPI_ID_WIFI_SET_ENABLED:
      set_wifi_enabled(payload != NULL && len >= 1 && payload[0] != 0);
      return ESP_OK;

    case SPI_ID_WIFI_START:
      set_wifi_enabled(true);
      return ESP_OK;

    case SPI_ID_WIFI_STOP:
      s_wifi_enabled = false;
      save_wifi_enabled(false);
      return ESP_OK;

    // ---- System status: report the WiFi on/off state the UI gates on ----
    case SPI_ID_SYSTEM_STATUS: {
      if (out_payload != NULL) {
        spi_system_status_t st = {
            .wifi_active = s_wifi_enabled ? 1 : 0,
            .wifi_connected = 0,
            .bt_running = 0,
            .bt_initialized = 0,
        };
        memcpy(out_payload, &st, sizeof(st));
      }
      return ESP_OK;
    }

    case SPI_ID_SYSTEM_PING:
    case SPI_ID_SYSTEM_VERSION:
      return ESP_OK;

    // ---- Not implemented yet: fast benign failure so the UI shows "no data"
    //      rather than blocking. (Radio features light up over time.)
    default:
      return ESP_ERR_INVALID_STATE;
  }
}
