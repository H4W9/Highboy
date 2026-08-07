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

#include "tutti_frutti_spam.h"

#include "android_spam.h"
#include "apple_juice_spam.h"
#include "samsung_spam.h"
#include "sour_apple_spam.h"
#include "swift_pair_spam.h"

static const char *TAG = "TUTTI_FRUTTI_SPAM";

#define TUTTI_FRUTTI_ATTACK_COUNT 5

static int s_tutti_index = 0;

int generate_tutti_frutti_payload(uint8_t *buffer, size_t max_len) {
  int ret = 0;
  switch (s_tutti_index) {
    case 0:
      ret = generate_android_payload(buffer, max_len);
      break;
    case 1:
      ret = generate_samsung_payload(buffer, max_len);
      break;
    case 2:
      ret = generate_swift_pair_payload(buffer, max_len);
      break;
    case 3:
      ret = generate_sour_apple_payload(buffer, max_len);
      break;
    case 4:
      ret = generate_apple_juice_payload(buffer, max_len);
      break;
    default:
      ret = generate_apple_juice_payload(buffer, max_len);
      break;
  }

  s_tutti_index++;
  if (s_tutti_index > (TUTTI_FRUTTI_ATTACK_COUNT - 1))
    s_tutti_index = 0;

  return ret;
}
