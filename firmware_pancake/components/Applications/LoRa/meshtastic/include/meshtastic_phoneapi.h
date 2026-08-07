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

#ifndef MESHTASTIC_PHONEAPI_H
#define MESHTASTIC_PHONEAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief State machine positions for the Meshtastic PhoneAPI handshake.
 *
 * The order must match src/mesh/PhoneAPI.h from the official firmware.
 */
typedef enum {
  PA_STATE_IDLE = 0,
  PA_STATE_SEND_MY_INFO,
  PA_STATE_SEND_METADATA,
  PA_STATE_SEND_DEVICEUI_CONFIG,
  PA_STATE_SEND_CHANNELS,
  PA_STATE_SEND_CONFIG,
  PA_STATE_SEND_MODULECONFIG,
  PA_STATE_SEND_FILEMANIFEST,
  PA_STATE_SEND_OWN_NODEINFO,
  PA_STATE_SEND_OTHER_NODEINFOS,
  PA_STATE_SEND_COMPLETE_ID,
  PA_STATE_SEND_PACKETS,
  PA_STATE_COUNT
} phoneapi_state_t;

/**
 * @brief Initialize the PhoneAPI subsystem.
 *
 * Must be called once before any transport (BLE, WiFi) can exchange
 * frames. Allocates the FromRadio queue and resets the FSM.
 *
 * @param node_num  Our node number.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_NO_MEM if mutex allocation fails
 */
esp_err_t phoneapi_init(uint32_t node_num);

/**
 * @brief Feed a raw ToRadio frame received from the app.
 *
 * Detects want_config_id (triggers FSM) or packet (forwards to mesh TX).
 *
 * @param pb_data  Protobuf bytes without transport framing.
 * @param pb_len   Size of pb_data.
 * @return
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG on malformed input
 */
esp_err_t phoneapi_on_toradio(const uint8_t *pb_data, uint16_t pb_len);

/**
 * @brief Pull the next FromRadio frame for the transport to emit.
 *
 * Transports should call in a loop after a ToRadio arrives or when
 * FROMNUM is notified. While the FSM has pending states, the call
 * advances and returns the next produced frame.
 *
 * @param[out] out_buf  Destination buffer (recommended >= 512 bytes).
 * @param max_len       Capacity of out_buf.
 * @return Bytes written to out_buf, 0 if no frame is ready.
 */
uint16_t phoneapi_poll_fromradio(uint8_t *out_buf, uint16_t max_len);

/**
 * @brief Whether there is at least one FromRadio frame ready (for FROMNUM notify).
 */
bool phoneapi_has_data(void);

/**
 * @brief Enqueue a MeshPacket built by the mesh core as a FromRadio frame.
 *
 * Wraps the MeshPacket in FromRadio { id, packet } and pushes to the queue.
 *
 * @param mp_bytes  Serialized MeshPacket.
 * @param mp_len    Size of mp_bytes.
 */
void phoneapi_push_packet(const uint8_t *mp_bytes, uint16_t mp_len);

/**
 * @brief Reset the FSM and drain the queue on transport disconnect.
 *
 * Called by BLE on disconnect or by WiFi on TCP client close. The next
 * want_config_id from any transport will restart the handshake.
 */
void phoneapi_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_PHONEAPI_H
