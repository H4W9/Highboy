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

#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

// Start the OTA receiver task. It listens on UART0 (wired to the P4) for a
// firmware push from the P4, writes it to the inactive OTA partition with the
// esp_ota APIs, and reboots into the new app. The C5 keeps running its normal
// app the whole time - no ROM download mode involved.
esp_err_t ota_service_start(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_SERVICE_H
