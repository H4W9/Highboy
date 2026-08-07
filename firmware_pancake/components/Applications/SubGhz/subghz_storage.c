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

#include "subghz_storage.h"

#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"

#include "subghz_protocol_serializer.h"

static const char *TAG = "SUBGHZ_STORAGE";

#define DECODED_BUF_SIZE 1024
#define RAW_BUF_SIZE     4096

esp_err_t subghz_storage_init(void) {
  ESP_LOGI(TAG, "Storage API Initialized (Placeholder Mode)");
  return ESP_OK;
}

esp_err_t subghz_storage_save_decoded(const char *name,
                                      const subghz_data_t *data,
                                      uint32_t frequency,
                                      uint32_t te) {
  if (name == NULL || data == NULL) {
    ESP_LOGE(TAG, "Invalid arguments to save_decoded");
    return ESP_ERR_INVALID_ARG;
  }

  char *buf = malloc(DECODED_BUF_SIZE);
  if (buf == NULL) {
    ESP_LOGE(TAG, "Failed to allocate decoded buffer");
    return ESP_ERR_NO_MEM;
  }

  subghz_protocol_serialize_decoded(data, frequency, te, buf, DECODED_BUF_SIZE);

  ESP_LOGI(TAG, "Saving Decoded Signal to %s...", name);
  ESP_LOGI(TAG, "\n--- FILE CONTENT START (%s) ---\n%s--- FILE CONTENT END ---\n", name, buf);

  free(buf);
  return ESP_OK;
}

esp_err_t
subghz_storage_save_raw(const char *name, const int32_t *pulses, size_t count, uint32_t frequency) {
  if (name == NULL || pulses == NULL || count == 0) {
    ESP_LOGE(TAG, "Invalid arguments to save_raw");
    return ESP_ERR_INVALID_ARG;
  }

  char *buf = malloc(RAW_BUF_SIZE);
  if (buf == NULL) {
    ESP_LOGE(TAG, "Failed to allocate raw buffer");
    return ESP_ERR_NO_MEM;
  }

  subghz_protocol_serialize_raw(pulses, count, frequency, buf, RAW_BUF_SIZE);

  ESP_LOGI(TAG, "Saving RAW Signal to %s...", name);
  ESP_LOGI(TAG, "\n--- FILE CONTENT START (%s) ---\n%s--- FILE CONTENT END ---\n", name, buf);

  free(buf);
  return ESP_OK;
}
