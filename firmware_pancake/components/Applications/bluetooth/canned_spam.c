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

// BLE Spam Indices:
// 0: AppleJuice (Airpods, Beats, etc)
// 1: SourApple (AppleTV Setup, Phone Setup, etc)
// 2: SwiftPair (Microsoft Windows popups)
// 3: Samsung (Samsung Watch/Galaxy popups)
// 4: Android (Google Fast Pair)
// 5: Tutti-Frutti (Cycles through all above)

#include "canned_spam.h"

#include <string.h>

#include "esp_log.h"

#include "bluetooth_service.h"
#include "spi_bridge.h"
#include "spi_protocol.h"
#include "spi_session.h"

static const char *TAG = "CANNED_SPAM";

typedef enum {
  CANNED_SPAM_CAT_APPLE_JUICE = 0,
  CANNED_SPAM_CAT_SOUR_APPLE,
  CANNED_SPAM_CAT_SWIFT_PAIR,
  CANNED_SPAM_CAT_SAMSUNG,
  CANNED_SPAM_CAT_ANDROID,
  CANNED_SPAM_CAT_TUTTI_FRUTTI,
  CANNED_SPAM_CAT_COUNT
} canned_spam_category_t;

static const canned_spam_type_t CATEGORY_INFO[] = {
    {"AppleJuice"}, {"SourApple"}, {"SwiftPair"}, {"Samsung"}, {"Android"}, {"Tutti-Frutti"}};

static bool s_is_running = false;
static int s_current_category = -1;
static uint32_t s_session_id = SPI_SESSION_INVALID_ID;

int spam_get_attack_count(void) {
  return CANNED_SPAM_CAT_COUNT;
}

const canned_spam_type_t *spam_get_attack_type(int index) {
  if (index < 0 || index >= CANNED_SPAM_CAT_COUNT) {
    return NULL;
  }
  return &CATEGORY_INFO[index];
}

esp_err_t spam_start(int attack_index) {
  if (s_is_running) {
    return ESP_ERR_INVALID_STATE;
  }

  if (!bluetooth_service_is_running()) {
    ESP_LOGE(TAG, "Bluetooth service is not running");
    return ESP_FAIL;
  }

  if (attack_index < 0 || attack_index >= CANNED_SPAM_CAT_COUNT) {
    return ESP_ERR_NOT_FOUND;
  }

  uint8_t payload = (uint8_t)attack_index;
  s_session_id = spi_session_start(SPI_ID_BT_APP_SPAM, &payload, sizeof(payload), NULL, NULL);
  if (s_session_id == SPI_SESSION_INVALID_ID) {
    ESP_LOGE(TAG, "Failed to start spam on C5");
    return ESP_FAIL;
  }

  s_current_category = attack_index;
  s_is_running = true;

  ESP_LOGI(TAG, "Spam started for category: %s", CATEGORY_INFO[attack_index].name);
  return ESP_OK;
}

esp_err_t spam_stop(void) {
  if (!s_is_running) {
    return ESP_ERR_INVALID_STATE;
  }

  if (s_session_id != SPI_SESSION_INVALID_ID) {
    spi_session_stop(s_session_id);
    s_session_id = SPI_SESSION_INVALID_ID;
  }
  s_is_running = false;
  ESP_LOGI(TAG, "Spam stopped");
  return ESP_OK;
}
