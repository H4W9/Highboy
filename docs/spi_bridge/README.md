# TentacleOS - P4 ↔ C5 SPI Bridge

How the two microcontrollers in TentacleOS talk to each other.

This is the **architecture overview** that ties both sides together. For
side-specific detail and migration recipes see the component READMEs:
- `firmware_p4/components/Service/spi_bridge/README.md` (master, protocol spec,
  command reference, session lifecycle, stream transport)
- `firmware_c5/components/Service/spi_bridge/README.md` (slave)

---

## 1. Roles

TentacleOS runs on two chips with a clean split of responsibilities:

| Chip | Role | Owns |
|------|------|------|
| **ESP32-P4** | Main OS / "brain" | UI (LVGL display), apps, storage (micro-SD via SDMMC), USB, the SPI **master** |
| **ESP32-C5** | Radio co-processor | WiFi, Bluetooth (NimBLE), LoRa, the SPI **slave** |

The P4 has no native WiFi/BT radio, so every radio action (scan, connect,
sniff, attack, mesh, …) is a **command sent to the C5** over SPI. The C5
executes it on the radio and returns results / streams data back. Anything that
needs the micro-SD is routed from the C5 to the P4 over this same bridge - the
C5 stores only on its internal flash (LittleFS).

```
        ┌────────────────────┐   SPI (10 MHz, mode 0, DMA)   ┌────────────────────┐
        │      ESP32-P4      │  ── SCLK / MOSI / MISO / CS ──►│      ESP32-C5      │
        │   (master / OS)    │  ◄──────── IRQ ───────────────│  (slave / radio)   │
        │                    │  ── UART1 + RESET/BOOT ───────►│  (firmware flash)  │
        └────────────────────┘                                └────────────────────┘
```

---

## 2. Physical layer

Two independent links connect the chips:

### 2.1 SPI bridge (runtime data path)

Standard **4-wire SPI, 1-bit, full-duplex, mode 0, 10 MHz, DMA-driven**
(`SPI_DMA_CH_AUTO` on both sides). The P4 is master, the C5 is slave. A separate
GPIO line (**IRQ**) lets the slave signal "response ready" to the master.

| Signal | P4 GPIO | C5 GPIO |
|--------|---------|---------|
| SCLK   | 20      | 6       |
| MOSI   | 21      | 7       |
| MISO   | 22      | 2       |
| CS     | 23      | 10      |
| IRQ    | 2       | 3       |

- **DMA** is mandatory: frames are 264 B (and stream frames 2 KB), far above the
  SPI hardware FIFO (~64 B). DMA also frees the CPU during transfers.
- Because of DMA, **every transfer length must be a multiple of 4 bytes** - see
  the frame sizing notes below.

### 2.2 UART + control (firmware flashing only)

The P4 flashes the C5's firmware over a separate UART link using the official
`esp-serial-flasher` component. Not used at runtime.

| Signal | P4 GPIO | C5 |
|--------|---------|----|
| UART TX (P4→C5) | 46 | GPIO12 (U0RXD) |
| UART RX (C5→P4) | 47 | GPIO11 (U0TXD) |
| RESET  | 48 | EN |
| BOOT   | 33 | IO0 (GPIO0 strapping) |

---

## 3. Frame format

Every packet on the SPI bus starts with a fixed **5-byte header**:

```c
typedef struct {
  uint8_t sync;     // 0xAA
  uint8_t type;     // 0x01 CMD, 0x02 RESP, 0x03 STREAM
  uint8_t category; // spi_cat_t - subsystem
  uint8_t op;       // operation within the category
  uint8_t length;   // payload bytes that follow (0-255)
} spi_header_t;
```

`SPI_FRAME_SIZE` = header + 256 B payload, **rounded up to a multiple of 4** for
DMA = **264 B**. The command/response path always transfers `SPI_FRAME_SIZE`.

### Command identifier = `category` + `op`

A command is identified by two header bytes that pack into a single 16-bit value
in code via `SPI_CMD(cat, op)`. The C5 routes to a dispatcher by `category`
alone; `op` selects the operation within it.

| Category | Value | Routed to |
|----------|-------|-----------|
| `SPI_CAT_SYSTEM`  | `0x00` | inline system handlers |
| `SPI_CAT_WIFI`    | `0x01` | `wifi_dispatcher` |
| `SPI_CAT_BT`      | `0x02` | `bt_dispatcher` |
| `SPI_CAT_LORA`    | `0x03` | (lora) |
| `SPI_CAT_MESH`    | `0x04` | meshtastic (split BLE/WiFi transport) |
| `SPI_CAT_MCORE`   | `0x05` | meshcore → `bt_dispatcher` |
| `SPI_CAT_SESSION` | `0xFF` | inline session handlers |

In C, the `SPI_ID_*` constants stay single named values (e.g.
`SPI_ID_WIFI_SCAN = SPI_CMD(SPI_CAT_WIFI, 0x10) = 0x0110`), so call sites and
dispatcher `case` labels are unchanged - only the wire carries the two bytes.
The full command table lives in the P4 component README.

### Response status

A `RESP` frame's **payload byte 0 is the status** (`spi_status_t`): `OK (0)`,
`BUSY (1)`, `ERROR (2)`, `UNSUPPORTED (3)`, `INVALID_ARG (4)`; the rest of the
payload is the response data.

---

## 4. Command / response flow

The bridge is a master-driven request/response protocol with an IRQ handshake:

```
P4 (master)                                  C5 (slave)
   │  clock CMD frame (264 B)  ───────────►   receive into armed RX buffer
   │                                          route by category → dispatcher
   │                                          build RESP, arm TX buffer
   │  ◄────────── IRQ rising edge ──────────  pulse IRQ (~10 µs)
   │  clock again to read RESP (264 B) ───►   transmit RESP
   │  parse status + payload
```

- The P4 catches the IRQ via a **GPIO rising-edge interrupt** (ISR → semaphore),
  so the C5 only needs a short (~10 µs) pulse - no held level, no millisecond
  delay.
- The C5's `bridge_task` keeps a **receive transaction always armed in hardware**
  (it queues the next RX before the current response finishes), so a command is
  never missed in the gap between transfers, even under task preemption.
- A per-command **mutex** on the P4 serialises commands; long radio ops get
  longer timeouts (`SPI_TIMEOUT_WIFI_MS = 20 s`, default `1 s`).

---

## 5. Generic data pipe (pulling lists)

Operations that produce lists (scan results, etc.) don't push everything at
once. The C5 points the bridge at its result array via
`spi_bridge_provide_results(ptr, count, item_size)`, and the P4 pulls items with
`SPI_ID_SYSTEM_DATA` using special indices:

| Index | Meaning |
|-------|---------|
| `0xFFFF` | item count |
| `0..N-1` | one item |
| `0xEEEE` | live `spi_sniffer_stats_t` |
| `0xDDDD` | deauth counter |

This is also how the **Packet Monitor** works: it's a counter-only sniffer mode
that just polls the stats - it does not stream frames.

---

## 6. Streaming (live data, e.g. pcap)

Long-running ops that emit a continuous feed (WiFi/BLE sniffers, mesh phone
bridge) use a stream path. The C5 buffers records in a 64-deep ring; the P4
drains them by polling `SPI_ID_SYSTEM_STREAM`.

To keep throughput high, the transport **batches many records into one large
transfer** (`SPI_STREAM_FRAME_SIZE = 2048 B`) instead of one record per
round-trip:

```
STREAM frame payload (after the 5-byte header, type = STREAM):
  [u16 batch_len][record][record]...        record = [u16 op][u8 len][len bytes]
```

- `batch_len = 0` ⇒ no data pending ⇒ the P4 backs off and polls later.
- The P4 unpacks and dispatches **each record to its op's callback**, exactly as
  if it had arrived in its own frame - so session/`seq`/backpressure semantics
  stay **per record**.
- The command/response path is untouched (still `SPI_FRAME_SIZE`).

**Throughput:** the original one-record-per-frame + 1 ms IRQ pulse capped streams
at ~120 KB/s. Shortening the IRQ pulse (~3×) plus batching lifts the ceiling to
roughly ~1 MB/s at 10 MHz - enough for dense-AP / targeted capture. A saturated
data channel can still overrun it (physics on a 1-bit link), in which case
records are **dropped and counted** (capture is never blocked) - the right tool
there is a capture filter.

---

## 7. Session lifecycle (anti-zombie + backpressure)

Streaming/long-running ops are wrapped in a **session** so the C5 never keeps
running into the void if the P4 crashes or stops listening:

1. **Session ID** - the C5 returns a random 32-bit `session_id` on START; both
   sides track it, and stream records carry it so stale data is discarded.
2. **Heartbeat** - the P4 sends `SPI_ID_SESSION_HEARTBEAT` every **2 s** with its
   `last_acked_seq`. A C5 watchdog (1 s tick) kills any session whose last
   heartbeat is older than **5 s** and emits `SPI_ID_SESSION_LOST`.
3. **Backpressure window** - each record carries `{session_id, seq}`. The C5
   refuses to emit when `seq - last_acked_seq >= SPI_SESSION_WINDOW (64)`,
   preventing overflow when the radio produces faster than the bridge drains.

| Direction | When | Packet |
|-----------|------|--------|
| P4→C5 | START | `op_id` + params |
| C5→P4 | START reply | status + `spi_session_resp_t { session_id }` |
| P4→C5 | every 2 s | `SPI_ID_SESSION_HEARTBEAT` + `{ session_id, last_acked_seq }` |
| C5→P4 | data | batched STREAM frame (§6); each record = `op` + meta + payload |
| P4→C5 | STOP | `SPI_ID_SESSION_STOP` + `{ session_id }` |
| C5→P4 | watchdog kill | `SPI_ID_SESSION_LOST` + `{ session_id, cmd }` |

---

## 8. Firmware versioning & flashing

The C5 firmware is **embedded in the P4 firmware** at build time (bootloader +
partition table + app). On boot, `bridge_manager` queries the C5's version
(`SPI_ID_SYSTEM_VERSION`) and compares it against the P4's expected version
(`FIRMWARE_VERSION`, currently **1.3.0**). On mismatch (or no response) the P4
re-flashes the C5 over the UART link using `esp-serial-flasher`, writing the
full image:

| Image | C5 flash offset |
|-------|-----------------|
| bootloader | `0x2000` |
| partition table | `0x8000` |
| app | `0x10000` |

Any breaking change to the wire format must bump **both** versions
(`FIRMWARE_VERSION` on the P4 and `SPI_FW_VERSION_STRING` on the C5) to the same
new value, forcing a re-sync.

---

## 9. Key source files

**P4 (master)**
- `components/Service/spi_bridge/` - `spi_bridge.c` (send command, stream task),
  `spi_session.c` (session/heartbeat), `spi_protocol.h` (shared contract)
- `components/Drivers/spi_bridge_phy/` - SPI master PHY + IRQ edge ISR
- `components/Service/bridge_manager/` - version check + C5 recovery
- `components/Service/c5_flasher/` - `esp-serial-flasher` wrapper

**C5 (slave)**
- `components/Service/spi_bridge/` - `spi_bridge.c` (`bridge_task` routing +
  always-armed RX + stream batching), `wifi_dispatcher.c`, `bt_dispatcher.c`,
  `session_manager.c`, `spi_protocol.h`
- `components/Drivers/spi_slave/` - SPI slave driver (queued transactions)

`spi_protocol.h` is kept in sync between the two firmwares (the P4 copy is a
superset - it has port-scan commands the C5 doesn't implement).

---

## 10. Design constraints & limits

- **1-bit SPI** - dual/quad isn't wired, so the raw ceiling is the clock
  (~1.25 MB/s at 10 MHz). Higher clocks (20/40 MHz) are possible but limited by
  the SPI slave timing and trace integrity.
- **264 B / 2 KB frames must stay 4-byte aligned** for DMA.
- The two `spi_protocol.h` copies are maintained by hand - keep them in sync.
- Command `op` values currently reuse the legacy single-byte ids (e.g. WiFi ops
  start at `0x10`); renumbering to `0x01`-based per category is a safe cosmetic
  follow-up.
