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

#include "rfid_storage.h"

#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "RFID_STORAGE";

#define NVS_NAMESPACE  "rfid_cards"
#define NVS_KEY_COUNT  "count"
#define NVS_KEY_PREFIX "card_"
#define NVS_KEY_MAX    16

static int s_count = 0;
static bool s_is_init = false;

static void build_key(int index, char *out_key) {
  snprintf(out_key, NVS_KEY_MAX, "%s%d", NVS_KEY_PREFIX, index);
}

void rfid_storage_init(void) {
  if (s_is_init) {
    return;
  }

  nvs_handle_t handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
  if (ret == ESP_OK) {
    int32_t count = 0;
    if (nvs_get_i32(handle, NVS_KEY_COUNT, &count) == ESP_OK) {
      s_count = (int)count;
    }
    nvs_close(handle);
  }

  s_is_init = true;
  ESP_LOGI(TAG, "Initialized — %d entries", s_count);
}

int rfid_storage_count(void) {
  return s_count;
}

esp_err_t rfid_storage_save(const char *name,
                            const ys_rfid2_raw_data_t *raw,
                            const rfid_decoded_data_t *decoded,
                            bool is_decoded) {
  if (name == NULL || raw == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_count >= RFID_STORAGE_MAX_ENTRIES) {
    ESP_LOGW(TAG, "Storage full (%d entries)", s_count);
    return ESP_ERR_NO_MEM;
  }

  rfid_storage_entry_t entry = {0};
  strncpy(entry.name, name, RFID_STORAGE_NAME_MAX - 1);
  entry.raw = *raw;
  entry.is_decoded = is_decoded;
  if (is_decoded && decoded != NULL) {
    entry.decoded = *decoded;
  }

  nvs_handle_t handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
    return ESP_FAIL;
  }

  char key[NVS_KEY_MAX];
  build_key(s_count, key);

  ret = nvs_set_blob(handle, key, &entry, sizeof(entry));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write entry: %s", esp_err_to_name(ret));
    nvs_close(handle);
    return ESP_FAIL;
  }

  s_count++;
  nvs_set_i32(handle, NVS_KEY_COUNT, (int32_t)s_count);
  nvs_commit(handle);
  nvs_close(handle);

  ESP_LOGI(TAG, "Saved '%s' at index %d", name, s_count - 1);
  return ESP_OK;
}

esp_err_t rfid_storage_load(int index, rfid_storage_entry_t *out_entry) {
  if (out_entry == NULL || index < 0 || index >= s_count) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
  if (ret != ESP_OK) {
    return ESP_FAIL;
  }

  char key[NVS_KEY_MAX];
  build_key(index, key);

  size_t len = sizeof(rfid_storage_entry_t);
  ret = nvs_get_blob(handle, key, out_entry, &len);
  nvs_close(handle);

  if (ret != ESP_OK) {
    return ESP_ERR_NOT_FOUND;
  }

  return ESP_OK;
}

esp_err_t rfid_storage_get_info(int index, rfid_storage_info_t *out_info) {
  if (out_info == NULL || index < 0 || index >= s_count) {
    return ESP_ERR_INVALID_ARG;
  }

  rfid_storage_entry_t entry = {0};
  esp_err_t ret = rfid_storage_load(index, &entry);
  if (ret != ESP_OK) {
    return ret;
  }

  strncpy(out_info->name, entry.name, RFID_STORAGE_NAME_MAX - 1);
  out_info->protocol_name = entry.is_decoded ? entry.decoded.protocol_name : "Unknown";
  out_info->card_number = entry.decoded.card_number;
  out_info->facility_code = entry.decoded.facility_code;

  return ESP_OK;
}

int rfid_storage_list(rfid_storage_info_t *out_infos, int max) {
  if (out_infos == NULL) {
    return 0;
  }

  int listed = 0;
  for (int i = 0; i < s_count && listed < max; i++) {
    if (rfid_storage_get_info(i, &out_infos[listed]) == ESP_OK) {
      listed++;
    }
  }

  return listed;
}

esp_err_t rfid_storage_delete(int index) {
  if (index < 0 || index >= s_count) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (ret != ESP_OK) {
    return ESP_FAIL;
  }

  // Shift entries down to fill the gap
  for (int i = index; i < s_count - 1; i++) {
    rfid_storage_entry_t entry = {0};
    char src_key[NVS_KEY_MAX];
    char dst_key[NVS_KEY_MAX];
    build_key(i + 1, src_key);
    build_key(i, dst_key);

    size_t len = sizeof(rfid_storage_entry_t);
    if (nvs_get_blob(handle, src_key, &entry, &len) == ESP_OK) {
      nvs_set_blob(handle, dst_key, &entry, sizeof(entry));
    }
  }

  // Remove last entry
  char last_key[NVS_KEY_MAX];
  build_key(s_count - 1, last_key);
  nvs_erase_key(handle, last_key);

  s_count--;
  nvs_set_i32(handle, NVS_KEY_COUNT, (int32_t)s_count);
  nvs_commit(handle);
  nvs_close(handle);

  ESP_LOGI(TAG, "Deleted index %d, %d entries remaining", index, s_count);
  return ESP_OK;
}

esp_err_t rfid_storage_find_by_id(const char *id_str, int *out_index) {
  if (id_str == NULL || out_index == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  for (int i = 0; i < s_count; i++) {
    rfid_storage_entry_t entry = {0};
    if (rfid_storage_load(i, &entry) == ESP_OK) {
      if (strcmp(entry.raw.id_str, id_str) == 0) {
        *out_index = i;
        return ESP_OK;
      }
    }
  }

  return ESP_ERR_NOT_FOUND;
}
