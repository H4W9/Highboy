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

#include "rfid_protocol_registry.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "RFID_REGISTRY";

extern rfid_protocol_t protocol_em4100;
extern rfid_protocol_t protocol_hid;
extern rfid_protocol_t protocol_hid_corp;
extern rfid_protocol_t protocol_indala;
extern rfid_protocol_t protocol_awid;
extern rfid_protocol_t protocol_fdxb;
extern rfid_protocol_t protocol_viking;
extern rfid_protocol_t protocol_pyramid;
extern rfid_protocol_t protocol_keri;
extern rfid_protocol_t protocol_pac_stanley;
extern rfid_protocol_t protocol_paradox;
extern rfid_protocol_t protocol_jablotron;
extern rfid_protocol_t protocol_kantech;
extern rfid_protocol_t protocol_gallagher;
extern rfid_protocol_t protocol_nexwatch;
extern rfid_protocol_t protocol_noralsy;
extern rfid_protocol_t protocol_securakey;

static const rfid_protocol_t *REGISTERED_PROTOCOLS[] = {
    &protocol_em4100,
    &protocol_hid,
    &protocol_hid_corp,
    &protocol_indala,
    &protocol_awid,
    &protocol_fdxb,
    &protocol_viking,
    &protocol_pyramid,
    &protocol_keri,
    &protocol_pac_stanley,
    &protocol_paradox,
    &protocol_jablotron,
    &protocol_kantech,
    &protocol_gallagher,
    &protocol_nexwatch,
    &protocol_noralsy,
    &protocol_securakey,
};

#define REGISTERED_PROTOCOLS_COUNT (sizeof(REGISTERED_PROTOCOLS) / sizeof(REGISTERED_PROTOCOLS[0]))

void rfid_protocol_registry_init(void) {}

bool rfid_protocol_registry_decode_all(const ys_rfid2_raw_data_t *raw,
                                       rfid_decoded_data_t *out_data) {
  for (size_t i = 0; i < REGISTERED_PROTOCOLS_COUNT; i++) {
    if (REGISTERED_PROTOCOLS[i]->decode != NULL && REGISTERED_PROTOCOLS[i]->decode(raw, out_data)) {
      ESP_LOGD(TAG, "Decoded protocol: %s", REGISTERED_PROTOCOLS[i]->name);
      return true;
    }
  }
  return false;
}

const rfid_protocol_t *rfid_protocol_registry_get_by_name(const char *name) {
  if (name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < REGISTERED_PROTOCOLS_COUNT; i++) {
    if (strcmp(REGISTERED_PROTOCOLS[i]->name, name) == 0) {
      return REGISTERED_PROTOCOLS[i];
    }
  }
  return NULL;
}

size_t rfid_protocol_registry_get_count(void) {
  return REGISTERED_PROTOCOLS_COUNT;
}

const rfid_protocol_t *rfid_protocol_registry_get_by_index(size_t index) {
  if (index >= REGISTERED_PROTOCOLS_COUNT) {
    return NULL;
  }
  return REGISTERED_PROTOCOLS[index];
}
