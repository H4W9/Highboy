# SPI Bridge - P4 Master

This component manages the high-speed communication link between the **ESP32-P4 (Main OS)** and the **ESP32-C5 (Radio Co-processor)**.

## Architecture
The P4 acts as the **SPI Master**. It is responsible for:
1. Generating the SCLK and managing the CS line.
2. Initiating all command transfers.
3. Handling the **IRQ (Handshake)** signal from the C5 to know when response data is ready.
4. Managing the C5 lifecycle (Reset, Boot mode, and Firmware Updates via UART).

## Protocol Specification
Every packet follows a 4-byte fixed header:
- `Sync (0xAA)`: Packet synchronization.
- `Type`: `0x01` (Command), `0x02` (Response), `0x03` (Stream).
- `ID`: Function identifier (defined in `spi_protocol.h`).
- `Length`: Size of the following payload (0-255 bytes).

## Generic Data Pipe
To keep the bridge simple, we use a "Dumb Pipe" approach for large data sets (like Scan results):
1. **Pull Count**: Call `SPI_ID_SYSTEM_DATA` with index `0xFFFF`.
2. **Pull Item**: Call `SPI_ID_SYSTEM_DATA` with index `0 to N`.
3. **Real-time Stats**: Call `SPI_ID_SYSTEM_DATA` with index `0xEEEE` to get a `sniffer_stats_t` structure.

## Adding a New Command
To add a new feature (e.g., "GPS Get Location"):

1. **Protocol**: Add `SPI_ID_GPS_GET` to `spi_protocol.h`.
2. **C5 Dispatcher**:
   - Open `wifi_dispatcher.c` (or a new `gps_dispatcher.c`).
   - Add the case for `SPI_ID_GPS_GET`.
   - Call the actual hardware driver.
   - If it returns a list, call `spi_bridge_provide_results(pointer, count, size)`.
3. **P4 Wrapper**:
   - Create a wrapper in `Applications` or `Service`.
   - Use `spi_bridge_send_command(SPI_ID_GPS_GET, ...)` to trigger the action.
   - Use the generic `SPI_ID_SYSTEM_DATA` to pull results if necessary.

## Session Lifecycle (Long-Running Operations)

For operations that run for an extended period (sniffers, monitors, attacks
that emit a stream of events), the basic request-response model is unsafe:
if the master dies or stops listening, the slave keeps running indefinitely
and sends data into the void. The session protocol fixes this with three
mechanisms working together:

### 1. Session ID
Every long-running operation is tagged with a 32-bit `session_id` chosen
randomly by the C5 when the operation starts. Both sides track the active
session; stream packets carry the id so stale data can be discarded after
a restart.

### 2. Heartbeat (anti-zombie)
The P4 sends `SPI_ID_SESSION_HEARTBEAT { session_id, last_acked_seq }`
every **2 seconds** while a session is active. The C5 has a watchdog task
that runs every second and kills any session whose last heartbeat is older
than **5 seconds**. When killed, the C5 emits `SPI_ID_SESSION_LOST` as a
stream so the master can react (e.g., restart, show error UI).

If the master detects 3 consecutive heartbeat failures, it assumes the
session is gone and fires its local `on_lost` callback.

### 3. Backpressure window
Stream packets carry `{ session_id, seq }`. The master accumulates
`last_acked_seq` and reports it via heartbeat. The C5 refuses to emit if
`seq - last_acked_seq >= SPI_SESSION_WINDOW (64)` — protects against
buffer overflow when the slave produces faster than the master drains.
Drops are counted and logged.

### Wire shapes

| Direction | When | Packet |
|-----------|------|--------|
| P4 → C5 | START | `op_id` + op-specific params |
| C5 → P4 | START reply | status byte + `spi_session_resp_t { session_id }` |
| P4 → C5 | every 2s | `SPI_ID_SESSION_HEARTBEAT` + `spi_heartbeat_req_t` |
| C5 → P4 | heartbeat reply | status + `spi_heartbeat_resp_t { alive }` |
| C5 → P4 | data | `op_id` STREAM + `spi_stream_meta_t { session_id, seq }` + payload |
| P4 → C5 | STOP | `SPI_ID_SESSION_STOP` + `spi_session_stop_req_t { session_id }` |
| C5 → P4 | watchdog kill | `SPI_ID_SESSION_LOST` STREAM + `spi_session_lost_t { session_id, op_id }` |

### Master API

```c
// Start a long-running operation. Spawns heartbeat task internally.
uint32_t spi_session_start(spi_id_t op_id,
                           const uint8_t *params, uint8_t params_len,
                           spi_session_stream_cb_t on_stream,  // peeled meta
                           spi_session_lost_cb_t on_lost);

// Clean teardown. Kills heartbeat, sends STOP.
esp_err_t spi_session_stop(uint32_t session_id);
```

Returns `SPI_SESSION_INVALID_ID` (0) on START failure. The `on_stream`
callback receives the **operation payload only** — the meta header is
stripped and ack tracking is invisible to the consumer.

### Slave API (C5)

```c
// Open a session for the op_id. Closes any prior session first.
uint32_t session_manager_start(spi_id_t op_id, session_kill_cb_t kill_cb);

// Emit a stream packet (prefixes meta, applies backpressure).
esp_err_t session_manager_try_emit(uint32_t session_id,
                                   const uint8_t *data, uint8_t len);
```

The op implementation stores the returned `session_id` and uses it for
every emit. The `kill_cb` is invoked by the watchdog if heartbeats stop —
the op should call its own `_stop()` from there.

### Migrating a New Operation (recipe)

There are two patterns depending on whether the op emits streams. Both
are used in the codebase — see `wifi_sniffer` (streaming) and
`wifi_deauther` (non-streaming) as references.

#### Pattern A — Non-streaming op (deauther, flood, evil_twin, …)

The op runs in background but does NOT emit packets to the master. The
master polls for results via `SPI_ID_SYSTEM_DATA` if it needs data.

**C5 side (only the dispatcher changes — op .c/.h untouched):**
```c
// In wifi_dispatcher.c (or bt_dispatcher.c):
static void killed_my_op(spi_id_t id) { (void)id; my_op_stop(); }

case SPI_ID_MY_OP:
    if (!my_op_start(...)) return SPI_STATUS_ERROR;
    return open_session(SPI_ID_MY_OP, killed_my_op,
                        out_resp_payload, out_resp_len, my_op_stop);
```

**P4 side (wrapper):**
```c
static uint32_t s_session_id = SPI_SESSION_INVALID_ID;

bool my_op_start(...) {
    s_session_id = spi_session_start(SPI_ID_MY_OP, params, len, NULL, NULL);
    return s_session_id != SPI_SESSION_INVALID_ID;
}

void my_op_stop(void) {
    if (s_session_id != SPI_SESSION_INVALID_ID) {
        spi_session_stop(s_session_id);
        s_session_id = SPI_SESSION_INVALID_ID;
    }
}
```

#### Pattern B — Streaming op (sniffer, ble_sniffer, …)

The op emits a continuous stream of packets to the master.

**C5 side:**
1. Add `static uint32_t s_session_id = SPI_SESSION_INVALID_ID;` to the
   op's `.c`.
2. Add public `_bind_session(uint32_t)` setter and
   `_session_killed(spi_id_t)` kill callback (the latter calls `_stop()`).
3. Replace `spi_bridge_stream_push(SPI_ID_OP, data, len)` with
   `session_manager_try_emit(s_session_id, data, len)`.
4. In the dispatcher, replace the START handler with: call
   `op_start(...)`, then `session_manager_start(SPI_ID_OP, op_session_killed)`,
   then `op_bind_session(sid)`, then return
   `spi_session_resp_t { sid }` as response payload.

**P4 side:**
1. Replace `spi_bridge_send_command(SPI_ID_OP, …)` +
   `spi_bridge_register_stream_cb(SPI_ID_OP, raw_cb)` with a single
   `spi_session_start(SPI_ID_OP, params, …, on_stream, on_lost)`.
2. Store the returned `session_id`.
3. Change STOP to `spi_session_stop(session_id)`.
4. The `on_stream` callback signature is
   `void(const uint8_t *payload, uint8_t len)` — the meta header is
   already stripped.

### Tunables
Defined in `session_manager.c` (slave) and `spi_session.c` (master):
- `SESSION_TIMEOUT_MS` = 5000 — slave watchdog timeout
- `WATCHDOG_PERIOD_MS` = 1000 — slave watchdog tick
- `HEARTBEAT_INTERVAL_MS` = 2000 — master ping period
- `HEARTBEAT_FAIL_LIMIT` = 3 — master fails before declaring lost
- `SPI_SESSION_WINDOW` = 64 — backpressure window (in `spi_protocol.h`)

### Migrated operations

All long-running ops now use the session lifecycle. Each one:
- Returns `spi_session_resp_t { session_id }` on START.
- Has a kill_cb registered with the session manager that calls its `_stop()`.
- Is closed by the master via `SPI_ID_SESSION_STOP { session_id }` (sent
  internally by `spi_session_stop`).
- Is auto-killed by the C5 watchdog if the master stops sending heartbeats
  for 5s (master crash, screen freeze, etc.).

| Op | C5 module | P4 wrapper | Streams? |
|----|-----------|-----------|----------|
| `WIFI_APP_SNIFFER` | wifi_sniffer.c | wifi_sniffer.c | ✓ stream |
| `BT_APP_SNIFFER` | ble_sniffer.c | bluetooth_service.c | ✓ stream |
| `WIFI_APP_DEAUTHER` | wifi_deauther.c | wifi_deauther.c | – |
| `WIFI_APP_FLOOD` | wifi_flood.c | wifi_flood.c | – |
| `WIFI_APP_EVIL_TWIN` | evil_twin.c | evil_twin.c | – |
| `WIFI_APP_BEACON_SPAM` | beacon_spam.c | beacon_spam.c | – |
| `WIFI_APP_DEAUTH_DET` | deauther_detector.c | deauther_detector.c | – |
| `WIFI_APP_PROBE_MON` | probe_monitor.c | probe_monitor.c | – |
| `WIFI_APP_SIGNAL_MON` | signal_monitor.c | signal_monitor.c | – |
| `BT_APP_FLOOD` | ble_connect_flood.c | ble_connect_flood.c | – |
| `BT_APP_SKIMMER` | skimmer_detector.c | skimmer_detector.c | – |
| `BT_APP_TRACKER` | tracker_detector.c | tracker_detector.c | – |
| `BT_APP_SPAM` | (handler pending) | canned_spam.c | – |
| `BT_APP_FLOOD` (L2CAP variant) | ble_connect_flood.c | ble_l2cap_flood.c | – |

The legacy `SPI_ID_WIFI_APP_ATTACK_STOP` and `SPI_ID_BT_APP_STOP` shotgun
commands have been removed entirely. Every op now stops via its own
session via `SPI_ID_SESSION_STOP { session_id }`.

## Hardware Hookup
| Signal | P4 Pin | C5 Pin |
|--------|--------|--------|
| SCLK   | 20     | 6      |
| MOSI   | 21     | 7      |
| MISO   | 22     | 2      |
| CS     | 23     | 10     |
| IRQ    | 2      | 3      |
| RESET  | 48     | EN     |
| BOOT   | 33     | IO0    |
| UART TX| 46     | RX     |
| UART RX| 47     | TX     |
