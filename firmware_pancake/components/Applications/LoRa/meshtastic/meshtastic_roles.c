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

#include "meshtastic_roles.h"

#include "esp_log.h"

#include "meshtastic_nvs.h"

static const char *TAG = "MESHTASTIC_ROLES";

#define MT_ROLE_NVS_KEY "role"
#define MT_REBR_NVS_KEY "rebr"

static const char *ROLE_NAMES[MT_ROLE_COUNT] = {
    "CLIENT",
    "CLIENT_MUTE",
    "ROUTER",
    "ROUTER_CLIENT",
    "REPEATER",
    "TRACKER",
    "SENSOR",
    "TAK",
    "CLIENT_HIDDEN",
    "LOST_AND_FOUND",
    "TAK_TRACKER",
    "ROUTER_LATE",
    "CLIENT_BASE",
};

static bool is_core_portnum(uint32_t portnum) {
  return portnum == 1 || portnum == 3 || portnum == 4 || portnum == 5 || portnum == 7 ||
         portnum == 67 || portnum == 70;
}

mt_role_t mt_role_current(void) {
  uint32_t value = mt_nvs_get_u32(MT_ROLE_NVS_KEY, (uint32_t)MT_ROLE_CLIENT);
  return (mt_role_t)value;
}

void mt_role_set(mt_role_t role) {
  mt_nvs_set_u32(MT_ROLE_NVS_KEY, (uint32_t)role);
  ESP_LOGI(TAG, "Role applied: %s", mt_role_name(role));

  mt_rebr_mode_t mode;
  if (role == MT_ROLE_ROUTER || role == MT_ROLE_ROUTER_LATE) {
    mode = MT_REBR_CORE_PORTNUMS_ONLY;
  } else if (role == MT_ROLE_CLIENT_HIDDEN) {
    mode = MT_REBR_LOCAL_ONLY;
  } else if (role == MT_ROLE_REPEATER) {
    mode = MT_REBR_ALL_SKIP_DECODING;
  } else {
    mode = MT_REBR_ALL;
  }
  mt_rebr_mode_set(mode);
}

const char *mt_role_name(mt_role_t role) {
  if (role < MT_ROLE_COUNT)
    return ROLE_NAMES[role];
  return "UNKNOWN";
}

mt_rebr_mode_t mt_rebr_mode_current(void) {
  uint32_t value = mt_nvs_get_u32(MT_REBR_NVS_KEY, (uint32_t)MT_REBR_ALL);
  return (mt_rebr_mode_t)value;
}

void mt_rebr_mode_set(mt_rebr_mode_t mode) {
  mt_nvs_set_u32(MT_REBR_NVS_KEY, (uint32_t)mode);
}

bool mt_role_is_rebroadcaster(mt_role_t role) {
  if (role == MT_ROLE_CLIENT_MUTE)
    return false;
  return true;
}

bool mt_role_cancels_on_duplicate(mt_role_t role) {
  if (role == MT_ROLE_ROUTER || role == MT_ROLE_ROUTER_LATE || role == MT_ROLE_REPEATER) {
    return false;
  }
  return true;
}

bool mt_role_is_late_rebroadcaster(mt_role_t role) {
  return role == MT_ROLE_ROUTER_LATE;
}

bool mt_rebr_should_forward(mt_rebr_mode_t mode, uint32_t portnum, bool is_from_known) {
  switch (mode) {
    case MT_REBR_NONE:
      return false;
    case MT_REBR_LOCAL_ONLY:
      return portnum != 0;
    case MT_REBR_KNOWN_ONLY:
      return is_from_known;
    case MT_REBR_CORE_PORTNUMS_ONLY:
      return is_core_portnum(portnum);
    case MT_REBR_ALL_SKIP_DECODING:
    case MT_REBR_ALL:
    default:
      return true;
  }
}

uint32_t mt_role_nodeinfo_interval_secs(mt_role_t role) {
  switch (role) {
    case MT_ROLE_ROUTER:
    case MT_ROLE_ROUTER_LATE:
    case MT_ROLE_REPEATER:
      return 10800;
    case MT_ROLE_TRACKER:
    case MT_ROLE_TAK_TRACKER:
      return 43200;
    case MT_ROLE_SENSOR:
      return 43200;
    case MT_ROLE_CLIENT_HIDDEN:
      return 86400;
    case MT_ROLE_CLIENT_MUTE:
      return 0;
    default:
      return 900;
  }
}

uint32_t mt_role_telemetry_interval_secs(mt_role_t role) {
  switch (role) {
    case MT_ROLE_SENSOR:
      return 900;
    case MT_ROLE_TRACKER:
    case MT_ROLE_TAK_TRACKER:
      return 1800;
    case MT_ROLE_ROUTER:
    case MT_ROLE_ROUTER_LATE:
    case MT_ROLE_REPEATER:
      return 3600;
    case MT_ROLE_CLIENT_HIDDEN:
    case MT_ROLE_CLIENT_MUTE:
      return 0;
    default:
      return 1800;
  }
}

uint32_t mt_role_position_interval_secs(mt_role_t role) {
  switch (role) {
    case MT_ROLE_TRACKER:
    case MT_ROLE_TAK_TRACKER:
      return 60;
    case MT_ROLE_CLIENT:
    case MT_ROLE_CLIENT_BASE:
    case MT_ROLE_TAK:
    case MT_ROLE_LOST_AND_FOUND:
      return 900;
    case MT_ROLE_ROUTER:
    case MT_ROLE_ROUTER_LATE:
    case MT_ROLE_REPEATER:
    case MT_ROLE_CLIENT_HIDDEN:
    case MT_ROLE_CLIENT_MUTE:
    case MT_ROLE_SENSOR:
      return 0;
    default:
      return 900;
  }
}

bool mt_role_is_unmessagable(mt_role_t role) {
  return role == MT_ROLE_ROUTER || role == MT_ROLE_ROUTER_LATE || role == MT_ROLE_REPEATER;
}

bool mt_role_should_respond(mt_role_t role) {
  return role != MT_ROLE_CLIENT_HIDDEN && role != MT_ROLE_CLIENT_MUTE;
}
