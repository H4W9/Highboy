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

#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define SPI_SYNC_BYTE                 0xAA
#define SPI_MAX_PAYLOAD               256
#define SPI_RESP_STATUS_SIZE          1
#define SPI_BT_SPAM_ITEM_LEN          32
#define SPI_BT_SPAM_LIST_MAX          64
#define SPI_WIFI_SNIFFER_FILENAME_MAX 96

// Special indices for SPI_ID_SYSTEM_DATA
#define SPI_DATA_INDEX_COUNT        0xFFFF
#define SPI_DATA_INDEX_STATS        0xEEEE
#define SPI_DATA_INDEX_DEAUTH_COUNT 0xDDDD

/**
 * @brief SPI message types.
 */
typedef enum { SPI_TYPE_CMD = 0x01, SPI_TYPE_RESP = 0x02, SPI_TYPE_STREAM = 0x03 } spi_type_t;

/**
 * @brief SPI function/command identifiers.
 */
typedef enum {
  // System (0x01 - 0x0F)
  SPI_ID_SYSTEM_PING = 0x01,
  SPI_ID_SYSTEM_STATUS = 0x02,
  SPI_ID_SYSTEM_REBOOT = 0x03,
  SPI_ID_SYSTEM_VERSION = 0x04,
  SPI_ID_SYSTEM_DATA = 0x05,
  SPI_ID_SYSTEM_STREAM = 0x06,

  // WiFi Basic (0x10 - 0x1F)
  SPI_ID_WIFI_SCAN = 0x10,
  SPI_ID_WIFI_CONNECT = 0x11,
  SPI_ID_WIFI_DISCONNECT = 0x12,
  SPI_ID_WIFI_GET_STA_INFO = 0x13,
  SPI_ID_WIFI_SET_AP = 0x14,
  SPI_ID_WIFI_START = 0x15,
  SPI_ID_WIFI_STOP = 0x16,
  SPI_ID_WIFI_SAVE_AP_CONFIG = 0x17,
  SPI_ID_WIFI_SET_ENABLED = 0x18,
  SPI_ID_WIFI_SET_AP_PASSWORD = 0x19,
  SPI_ID_WIFI_SET_AP_MAX_CONN = 0x1A,
  SPI_ID_WIFI_SET_AP_IP = 0x1B,
  SPI_ID_WIFI_PROMISC_START = 0x1C,
  SPI_ID_WIFI_PROMISC_STOP = 0x1D,
  SPI_ID_WIFI_CH_HOP_START = 0x1E,
  SPI_ID_WIFI_CH_HOP_STOP = 0x1F,

  // WiFi Applications & Attacks (0x20 - 0x4F)
  SPI_ID_WIFI_APP_SCAN_AP = 0x20,
  SPI_ID_WIFI_APP_SCAN_CLIENT = 0x21,
  SPI_ID_WIFI_APP_BEACON_SPAM = 0x22,
  SPI_ID_WIFI_APP_DEAUTHER = 0x23,
  SPI_ID_WIFI_APP_FLOOD = 0x24,
  SPI_ID_WIFI_APP_SNIFFER = 0x25,
  SPI_ID_WIFI_APP_EVIL_TWIN = 0x26,
  SPI_ID_WIFI_APP_DEAUTH_DET = 0x27,
  SPI_ID_WIFI_APP_PROBE_MON = 0x28,
  SPI_ID_WIFI_APP_SIGNAL_MON = 0x29,
  SPI_ID_WIFI_SNIFFER_SET_SNAPLEN = 0x2B,
  SPI_ID_WIFI_SNIFFER_SET_VERBOSE = 0x2C,
  SPI_ID_WIFI_SNIFFER_SAVE_FLASH = 0x2D,
  SPI_ID_WIFI_SNIFFER_SAVE_SD = 0x2E,
  SPI_ID_WIFI_SNIFFER_FREE_BUFFER = 0x2F,
  SPI_ID_WIFI_SNIFFER_STREAM_SD = 0x30,
  SPI_ID_WIFI_SNIFFER_CLEAR_PMKID = 0x31,
  SPI_ID_WIFI_SNIFFER_GET_PMKID_BSSID = 0x32,
  SPI_ID_WIFI_SNIFFER_CLEAR_HANDSHAKE = 0x33,
  SPI_ID_WIFI_SNIFFER_GET_HANDSHAKE_BSSID = 0x34,
  SPI_ID_WIFI_DEAUTH_STATUS = 0x35,
  SPI_ID_WIFI_DEAUTH_SEND_RAW = 0x36,
  SPI_ID_WIFI_ASSOC_REQUEST = 0x37,
  SPI_ID_WIFI_DEAUTH_SEND_FRAME = 0x38,
  SPI_ID_WIFI_DEAUTH_SEND_BROADCAST = 0x39,
  SPI_ID_WIFI_TARGET_SCAN_START = 0x3A,
  SPI_ID_WIFI_TARGET_SCAN_STATUS = 0x3B,
  SPI_ID_WIFI_TARGET_SAVE_FLASH = 0x3C,
  SPI_ID_WIFI_TARGET_SAVE_SD = 0x3D,
  SPI_ID_WIFI_TARGET_FREE = 0x3E,
  SPI_ID_WIFI_PROBE_SAVE_FLASH = 0x3F,
  SPI_ID_WIFI_PROBE_SAVE_SD = 0x40,
  SPI_ID_WIFI_EVIL_TWIN_TEMPLATE = 0x41,
  SPI_ID_WIFI_EVIL_TWIN_HAS_PASSWORD = 0x42,
  SPI_ID_WIFI_EVIL_TWIN_GET_PASSWORD = 0x43,
  SPI_ID_WIFI_EVIL_TWIN_RESET_CAPTURE = 0x44,
  SPI_ID_WIFI_CLIENT_SAVE_FLASH = 0x45,
  SPI_ID_WIFI_CLIENT_SAVE_SD = 0x46,
  SPI_ID_WIFI_AP_SAVE_FLASH = 0x47,
  SPI_ID_WIFI_AP_SAVE_SD = 0x48,
  SPI_ID_WIFI_PORT_SCAN_TARGET_RANGE = 0x49,
  SPI_ID_WIFI_PORT_SCAN_TARGET_LIST = 0x4A,
  SPI_ID_WIFI_PORT_SCAN_NETWORK = 0x4B,
  SPI_ID_WIFI_PORT_SCAN_CIDR = 0x4C,
  SPI_ID_WIFI_PORT_SCAN_STOP = 0x4D,
  SPI_ID_WIFI_GET_MAC = 0x4E,
  SPI_ID_WIFI_GET_IP_INFO = 0x4F,
  SPI_ID_WIFI_EVIL_TWIN_TMPL_BEGIN = 0xA0,
  SPI_ID_WIFI_EVIL_TWIN_TMPL_CHUNK = 0xA1,

  // Bluetooth Basic (0x50 - 0x5F)
  SPI_ID_BT_SCAN = 0x50,
  SPI_ID_BT_CONNECT = 0x51,
  SPI_ID_BT_DISCONNECT = 0x52,
  SPI_ID_BT_GET_INFO = 0x53,
  SPI_ID_BT_INIT = 0x54,
  SPI_ID_BT_DEINIT = 0x55,
  SPI_ID_BT_START = 0x56,
  SPI_ID_BT_STOP = 0x57,
  SPI_ID_BT_SET_RANDOM_MAC = 0x58,
  SPI_ID_BT_START_ADV = 0x59,
  SPI_ID_BT_STOP_ADV = 0x5A,
  SPI_ID_BT_SET_MAX_POWER = 0x5B,
  SPI_ID_BT_TRACKER_START = 0x5C,
  SPI_ID_BT_TRACKER_STOP = 0x5D,
  SPI_ID_BT_GET_ADDR_TYPE = 0x5E,
  SPI_ID_BT_SAVE_ANNOUNCE_CFG = 0x5F,

  // Bluetooth Apps & Attacks (0x60 - 0x7F)
  SPI_ID_BT_APP_SCANNER = 0x60,
  SPI_ID_BT_APP_SNIFFER = 0x61,
  SPI_ID_BT_APP_SPAM = 0x62,
  SPI_ID_BT_APP_FLOOD = 0x63,
  SPI_ID_BT_APP_SKIMMER = 0x64,
  SPI_ID_BT_APP_TRACKER = 0x65,
  SPI_ID_BT_APP_GATT_EXP = 0x66,
  SPI_ID_BT_SPAM_LIST_LOAD = 0x68,
  SPI_ID_BT_SPAM_LIST_BEGIN = 0x69,
  SPI_ID_BT_SPAM_LIST_ITEM = 0x6A,
  SPI_ID_BT_SPAM_LIST_COMMIT = 0x6B,
  SPI_ID_BT_SCREEN_INIT = 0x6C,
  SPI_ID_BT_SCREEN_DEINIT = 0x6D,
  SPI_ID_BT_SCREEN_IS_ACTIVE = 0x6E,
  SPI_ID_BT_SCREEN_SEND_PARTIAL = 0x6F,
  SPI_ID_BT_L2CAP_STATUS = 0x70,
  SPI_ID_BT_HID_INIT = 0x71,
  SPI_ID_BT_HID_DEINIT = 0x72,
  SPI_ID_BT_HID_IS_CONNECTED = 0x73,
  SPI_ID_BT_HID_SEND_KEY = 0x74,

  // LoRa (0x80 - 0x8F)
  SPI_ID_LORA_RX = 0x80,
  SPI_ID_LORA_TX = 0x81,

  // Meshtastic phone bridge (0x90 - 0x97)
  SPI_ID_MESH_BLE_INIT = 0x90,
  SPI_ID_MESH_BLE_STOP = 0x91,
  SPI_ID_MESH_WIFI_INIT = 0x92,
  SPI_ID_MESH_WIFI_STOP = 0x93,
  SPI_ID_MESH_FROMRADIO_PUSH = 0x94,
  SPI_ID_MESH_LOG_PUSH = 0x95,
  SPI_ID_MESH_STATUS = 0x96,
  SPI_ID_MESH_TORADIO_STREAM = 0x97,

  // MeshCore phone bridge (0x98 - 0x9C)
  SPI_ID_MCORE_BLE_INIT = 0x98,
  SPI_ID_MCORE_BLE_STOP = 0x99,
  SPI_ID_MCORE_TX_PUSH = 0x9A,
  SPI_ID_MCORE_RX_STREAM = 0x9B,
  SPI_ID_MCORE_STATUS = 0x9C,

  // Session lifecycle (long-running operations)
  SPI_ID_SESSION_HEARTBEAT = 0xF0,
  SPI_ID_SESSION_LOST = 0xF1,
  SPI_ID_SESSION_STOP = 0xF2
} spi_id_t;

/**
 * @brief SPI response status codes (payload byte 0 for RESP type).
 */
typedef enum {
  SPI_STATUS_OK = 0x00,
  SPI_STATUS_BUSY = 0x01,
  SPI_STATUS_ERROR = 0x02,
  SPI_STATUS_UNSUPPORTED = 0x03,
  SPI_STATUS_INVALID_ARG = 0x04
} spi_status_t;

/**
 * @brief SPI frame header (4 bytes).
 */
typedef struct {
  uint8_t sync;
  uint8_t type;   // spi_type_t
  uint8_t id;     // spi_id_t
  uint8_t length; // Payload length
} spi_header_t;

// Session protocol — see spi_bridge/README.md "Session Lifecycle"
#define SPI_SESSION_INVALID_ID 0u
#define SPI_SESSION_WINDOW     64u

/** Sent by C5 as response payload to a long-running START command.
 *  Valid only if the SPI response status byte is SPI_STATUS_OK. */
typedef struct __attribute__((packed)) {
  uint32_t session_id;
} spi_session_resp_t;

/** Sent by P4 every ~2s to keep a session alive and ack streams received. */
typedef struct __attribute__((packed)) {
  uint32_t session_id;
  uint32_t last_acked_seq; // highest stream seq P4 has processed
} spi_heartbeat_req_t;

/** C5 reply to heartbeat. */
typedef struct __attribute__((packed)) {
  uint8_t alive; // 1 if session still active, 0 if not found / different op
} spi_heartbeat_resp_t;

/** Prefixed to every stream payload from a session-managed operation. */
typedef struct __attribute__((packed)) {
  uint32_t session_id;
  uint32_t seq;
} spi_stream_meta_t;

/** Sent by P4 in payload of long-running STOP commands. */
typedef struct __attribute__((packed)) {
  uint32_t session_id;
} spi_session_stop_req_t;

/** Stream emitted by C5 when a session is auto-killed by the watchdog. */
typedef struct __attribute__((packed)) {
  uint32_t session_id;
  uint8_t op_id; // spi_id_t of the lost operation
} spi_session_lost_t;

#define SPI_FRAME_SIZE (sizeof(spi_header_t) + SPI_MAX_PAYLOAD)

/**
 * @brief WiFi connect request payload.
 */
typedef struct {
  char ssid[32];
  char password[64];
} __attribute__((packed)) spi_wifi_connect_t;

/**
 * @brief WiFi AP configuration payload.
 */
typedef struct {
  char ssid[32];
  char password[64];
  uint8_t max_conn;
  char ip_addr[16];
  uint8_t enabled;
} __attribute__((packed)) spi_wifi_ap_config_t;

/**
 * @brief Bluetooth device info response payload.
 */
typedef struct {
  uint8_t mac[6];
  uint8_t running;
  uint8_t initialized;
  uint16_t connected_count;
} __attribute__((packed)) spi_bt_info_t;

/**
 * @brief Bluetooth announce configuration payload.
 */
typedef struct {
  char name[32];
  uint8_t max_conn;
} __attribute__((packed)) spi_bt_announce_config_t;

/**
 * @brief System status response payload.
 */
typedef struct {
  uint8_t wifi_active;
  uint8_t wifi_connected;
  uint8_t bt_running;
  uint8_t bt_initialized;
} __attribute__((packed)) spi_system_status_t;

/**
 * @brief WiFi sniffer statistics payload.
 */
typedef struct {
  uint32_t packets;
  uint32_t deauths;
  uint32_t buffer_usage;
  int8_t signal_rssi;
  bool handshake_captured;
  bool pmkid_captured;
} __attribute__((packed)) spi_sniffer_stats_t;

#define SPI_WIFI_SNIFFER_MAX_DATA (SPI_MAX_PAYLOAD - 4)

/**
 * @brief WiFi sniffer stream frame.
 */
typedef struct {
  int8_t rssi;
  uint8_t channel;
  uint8_t len;
  uint8_t data[0];
} __attribute__((packed)) spi_wifi_sniffer_frame_t;

/**
 * @brief BLE sniffer stream frame.
 */
typedef struct {
  uint8_t addr[6];
  uint8_t addr_type;
  int8_t rssi;
  uint8_t len;
  uint8_t data[31];
} __attribute__((packed)) spi_ble_sniffer_frame_t;

/**
 * @brief WiFi IP info response payload.
 */
typedef struct {
  uint8_t interface; // 0 = STA, 1 = AP
  uint8_t mac[6];
  uint32_t ip;
  uint32_t netmask;
  uint32_t gw;
} __attribute__((packed)) spi_wifi_ip_info_t;

/**
 * @brief Port scan request with target IP and port range.
 */
typedef struct {
  char ip[16];
  uint16_t start_port;
  uint16_t end_port;
  uint16_t max_results;
  uint8_t reserved[2];
} __attribute__((packed)) spi_port_scan_range_req_t;

/**
 * @brief Port scan request for a network IP range.
 */
typedef struct {
  char start_ip[16];
  char end_ip[16];
  uint16_t start_port;
  uint16_t end_port;
  uint16_t max_results;
  uint8_t scan_type; // 0 = port range, 1 = port list
  uint8_t reserved;
} __attribute__((packed)) spi_port_scan_network_req_t;

/**
 * @brief Port scan request using CIDR notation.
 */
typedef struct {
  char base_ip[16];
  uint8_t cidr;
  uint8_t scan_type; // 0 = port range, 1 = port list
  uint16_t start_port;
  uint16_t end_port;
  uint16_t max_results;
} __attribute__((packed)) spi_port_scan_cidr_req_t;

/**
 * @brief Port scan result record.
 */
typedef struct {
  char ip_str[16];
  uint16_t port;
  uint8_t protocol; // 0 = TCP, 1 = UDP
  uint8_t status;   // 0 = OPEN, 1 = OPEN_FILTERED
  char banner[64];
} __attribute__((packed)) spi_port_scan_result_t;

/**
 * @brief Meshtastic transport init payload.
 *
 * Sent with SPI_ID_MESH_BLE_INIT and SPI_ID_MESH_WIFI_INIT.
 */
typedef struct {
  uint32_t node_num;
} __attribute__((packed)) spi_mesh_init_t;

/**
 * @brief Chunk header for fragmented Meshtastic transport frames.
 *
 * Each StreamAPI protobuf frame is split into 1..255 chunks of up to
 * SPI_MESH_CHUNK_PAYLOAD_MAX bytes. Receivers reassemble per seq.
 */
typedef struct {
  uint8_t seq;
  uint8_t chunk_idx;
  uint8_t total_chunks;
  uint8_t flags;
} __attribute__((packed)) spi_mesh_chunk_hdr_t;

#define SPI_MESH_CHUNK_FLAG_LAST   0x01
#define SPI_MESH_PAYLOAD_LIMIT     255
#define SPI_MESH_CHUNK_PAYLOAD_MAX (SPI_MESH_PAYLOAD_LIMIT - sizeof(spi_mesh_chunk_hdr_t))

/**
 * @brief Meshtastic transport status payload.
 *
 * Returned by SPI_ID_MESH_STATUS.
 */
typedef struct {
  uint8_t ble_connected;
  uint8_t ble_subscribed;
  uint8_t tcp_clients;
  uint8_t logradio_subscribed;
  uint32_t fromnum_counter;
} __attribute__((packed)) spi_mesh_status_t;

/**
 * @brief MeshCore BLE init payload.
 *
 * Sent with SPI_ID_MCORE_BLE_INIT. The C5 builds the advertised name as
 * "<name_prefix>-XXXX" (last 4 hex of MAC) and uses `pin` as the static
 * passkey for SC + MITM + DISP_ONLY pairing.
 */
typedef struct {
  char name_prefix[16];
  uint32_t pin;
} __attribute__((packed)) spi_mcore_init_t;

/**
 * @brief MeshCore transport status payload.
 *
 * Returned by SPI_ID_MCORE_STATUS.
 */
typedef struct {
  uint8_t ble_connected;
  uint8_t ble_subscribed;
  uint8_t reserved[2];
} __attribute__((packed)) spi_mcore_status_t;

#ifdef __cplusplus
}
#endif

#endif // SPI_PROTOCOL_H
