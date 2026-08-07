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

#include "subghz_protocol_registry.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "PROTOCOL_REGISTRY";

extern subghz_protocol_t protocol_rcswitch;
extern subghz_protocol_t protocol_came;
extern subghz_protocol_t protocol_nice_flo;
extern subghz_protocol_t protocol_princeton;
extern subghz_protocol_t protocol_ansonic;
extern subghz_protocol_t protocol_chamberlain;
extern subghz_protocol_t protocol_holtek;
extern subghz_protocol_t protocol_liftmaster;
extern subghz_protocol_t protocol_linear;
extern subghz_protocol_t protocol_rossi;

static const subghz_protocol_t *REGISTERED_PROTOCOLS[] = {
    &protocol_rcswitch,
    &protocol_came,
    &protocol_nice_flo,
    &protocol_princeton,
    &protocol_ansonic,
    &protocol_chamberlain,
    &protocol_holtek,
    &protocol_liftmaster,
    &protocol_linear,
    &protocol_rossi,
};

#define REGISTERED_PROTOCOLS_COUNT (sizeof(REGISTERED_PROTOCOLS) / sizeof(REGISTERED_PROTOCOLS[0]))

void subghz_protocol_registry_init(void) {}

bool subghz_protocol_registry_decode_all(const int32_t *pulses,
                                         size_t count,
                                         subghz_data_t *out_data) {
  for (size_t i = 0; i < REGISTERED_PROTOCOLS_COUNT; i++) {
    if (REGISTERED_PROTOCOLS[i]->decode != NULL &&
        REGISTERED_PROTOCOLS[i]->decode(pulses, count, out_data)) {
      ESP_LOGD(TAG, "Decoded protocol: %s", REGISTERED_PROTOCOLS[i]->name);
      return true;
    }
  }
  return false;
}

const subghz_protocol_t *subghz_protocol_registry_get_by_name(const char *name) {
  if (name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < REGISTERED_PROTOCOLS_COUNT; i++) {
    if (strcmp(REGISTERED_PROTOCOLS[i]->name, "RCSwitch") == 0 &&
        strstr(name, "RCSwitch") != NULL) {
      return REGISTERED_PROTOCOLS[i];
    }
    if (strcmp(REGISTERED_PROTOCOLS[i]->name, name) == 0) {
      return REGISTERED_PROTOCOLS[i];
    }
  }
  return NULL;
}
