> 📚 Canonical/aggregated copy in the project docs hub: [`docs/spi_bridge/c5.md`](../../../../docs/spi_bridge/c5.md).

# SPI Bridge - C5 Slave

This component transforms the **ESP32-C5** into a high-performance radio co-processor for the ESP32-P4.

## How it Works
The C5 runs a background task (`spi_bridge_task`) that stays in a blocked state waiting for the P4 to send SPI bytes. 

1. **Reception**: When bytes arrive, the task validates the `0xAA` sync byte.
2. **Routing**: It switches on the `Category` byte and routes the payload to the appropriate **Dispatcher** (WiFi or Bluetooth); the `Op` byte selects the operation within that dispatcher.
3. **Execution**: The Dispatcher executes the radio command (e.g., starts a scan).
4. **Notification**: Once the command is done (or results are ready), the C5 raises the **IRQ (Handshake)** pin.
5. **Response**: The P4 sees the IRQ, sends a dummy SPI clock, and the C5 "pushes" the response packet back.

## Memory Mapping (Zero-Copy Results)
The C5 uses a `current_data_source` pointer system. Instead of copying large scan lists into a bridge buffer, the Dispatcher simply points the bridge to the existing result array in memory:
```c
spi_bridge_provide_results(wifi_records, count, sizeof(wifi_ap_record_t));
```
The bridge then serves these items one by one when the P4 asks for them via the generic `SPI_ID_SYSTEM_DATA` command.

## Key Files
- `spi_bridge.c`: Main task and generic data provider logic.
- `wifi_dispatcher.c`: Logic to translate SPI IDs to WiFi driver calls.
- `bt_dispatcher.c`: Logic to translate SPI IDs to NimBLE/BT calls.
- `spi_slave_driver.c`: Low-level peripheral configuration.
- `session_manager.c`: Session lifecycle for long-running operations
  (heartbeat watchdog + backpressure). See "Session Lifecycle" below.

## Command Categories
The `Category` header byte (`spi_cat_t`) selects the subsystem; the `Op` byte
selects the operation within it. Together they pack into `spi_id_t` via
`SPI_CMD(cat, op)`.
- `0x00`: System/Bridge management (ping, status, version, data, stream).
- `0x01`: WiFi operations.
- `0x02`: Bluetooth operations.
- `0x03`: LoRa operations.
- `0x04`: Meshtastic phone bridge.
- `0x05`: MeshCore phone bridge.
- `0xFF`: Session lifecycle (heartbeat, lost, stop).

## Session Lifecycle (Long-Running Operations)

For full design and migration recipe, see the
[P4 README "Session Lifecycle" section](../../../../firmware_p4/components/Service/spi_bridge/README.md#session-lifecycle-long-running-operations).
The two sides share `spi_protocol.h` so the wire format is identical.

### Slave responsibilities (this side)

The `session_manager` runs a background watchdog that auto-kills sessions
when the master stops sending heartbeats (5s timeout). Each long-running
operation must:

1. Call `session_manager_start(op_id, kill_cb)` from its dispatcher case
   to obtain a `session_id`. The dispatcher returns this id to the master
   inside an `spi_session_resp_t` response payload.
2. Provide a `kill_cb(spi_id_t)` that calls the op's `_stop()` — invoked
   by the watchdog when the master goes quiet, and also when the master
   sends `SPI_ID_SESSION_STOP`.
3. **Streaming ops only**: store the id in the op (e.g. via a
   `_bind_session(uint32_t)` setter) and emit packets via
   `session_manager_try_emit(s_session_id, data, len)` instead of raw
   `spi_bridge_stream_push` — this prefixes meta and applies backpressure.

For non-streaming ops (deauther, flood, evil_twin, beacon_spam, etc.),
the `kill_cb` lives in the dispatcher itself — the op's `.c` file does
not need to know about sessions at all.

References:
- Streaming pattern: `wifi_sniffer.c`, `ble_sniffer.c`.
- Non-streaming pattern: see the `killed_*` static functions plus the
  `open_session()` / `bt_open_session()` helpers in the dispatchers.
