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

#include "ble_tracker.h"

#include "esp_log.h"

#include "bluetooth_service.h"

static const char *TAG = "BLE_TRACKER";

#define TRACKER_DEFAULT_RSSI -120

static int s_latest_rssi = TRACKER_DEFAULT_RSSI;

static void rssi_callback(int rssi) {
  s_latest_rssi = rssi;
}

esp_err_t ble_tracker_start(const uint8_t *addr) {
  s_latest_rssi = TRACKER_DEFAULT_RSSI;
  return bluetooth_service_start_tracker(addr, rssi_callback);
}

void ble_tracker_stop(void) {
  bluetooth_service_stop_tracker();
}

int ble_tracker_get_rssi(void) {
  return s_latest_rssi;
}
