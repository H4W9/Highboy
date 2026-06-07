# Host Link — C5 (BLE relay + log tee)

The companion app's **BLE transport terminates on the ESP32-C5** (it owns the BLE
radio). The C5 is a **transparent byte relay**: it ferries opaque host-link frames
to/from the P4 over the SPI bridge and forwards its own logs up. **All
crypto/auth lives on the P4** — the C5 never parses companion payloads.

Mirrors the proven Meshtastic/MeshCore phone-bridge pattern.

- Unified cross-firmware overview: [`docs/host-link.md`](../../../../docs/host-link.md)
- Wire format: [`docs/HOST_LINK_PROTOCOL.md`](../../../../docs/HOST_LINK_PROTOCOL.md)

This README is the **C5 component reference** (BLE relay + log tee).

## Files

| File | Role |
|------|------|
| `host_link_gatt.c` | NimBLE GATT server (NUS-style): a **write** char (app→device) and a **notify** char (device→app). "Just works" LE Secure Connections (no MITM). Splits notifications by ATT MTU; the app reassembles by frame `LEN`. |
| `host_transport.c` | Chunk/reassembly between BLE and SPI. BLE write → `SPI_ID_HOST_RX` stream (C5→P4). `SPI_ID_HOST_TX` chunks (P4→C5) → reassemble → BLE notify. Reuses `spi_mesh_chunk_hdr_t`. |
| `c5_log.c` | C5 log tee (`esp_log_set_vprintf`): keeps the local dev console, ANSI strip + level, drop-oldest ring, worker → `SPI_ID_SYSTEM_LOG` stream (C5→P4) as `[level u8][utf-8 text]`. |

## SPI ops (category `SPI_CAT_HOST = 0x06`, in `spi_protocol.h`)

| Op | Id | Direction | Purpose |
|----|----|-----------|---------|
| `SPI_ID_HOST_BLE_INIT` | `0x06A0` | P4→C5 cmd | start GATT + advertise (`spi_host_init_t { name_prefix }`) |
| `SPI_ID_HOST_BLE_STOP` | `0x06A1` | P4→C5 cmd | stop GATT |
| `SPI_ID_HOST_TX` | `0x06A2` | P4→C5 cmd (push) | device→app bytes → BLE notify |
| `SPI_ID_HOST_RX` | `0x06A3` | C5→P4 stream | app→device bytes (BLE write) |
| `SPI_ID_HOST_STATUS` | `0x06A4` | P4→C5 cmd | poll `spi_host_status_t { ble_connected, ble_subscribed }` |

`SPI_ID_SYSTEM_LOG` (`0x0007`, C5→P4 stream) carries the forwarded log lines.

## Dispatch

`SPI_CAT_HOST` is routed to `bt_dispatcher_execute` (alongside `SPI_CAT_BT` /
`SPI_CAT_MCORE`) in `spi_bridge.c`. The handlers call into `host_transport` /
`host_link_gatt`.

## Boot wiring (`kernel.c`)

`c5_log_init()` runs right after `spi_bridge_slave_init()` (it pushes to the SPI
stream). The GATT server is started on demand by the P4 (`SPI_ID_HOST_BLE_INIT`),
not at boot, so it doesn't hog NimBLE from the BLE attack features.

## Caveats

- **NimBLE is single-owner**: host-link BLE, MeshCore, and Meshtastic each refuse
  to init while another holds NimBLE.
- The C5 log stream is always enabled on this side; the P4 drops the resulting
  `LOG` frames when no companion session is active, and the **log-over-BLE**
  toggle (P4) gates BLE delivery. Build-validated; **not yet hardware-tested**.
