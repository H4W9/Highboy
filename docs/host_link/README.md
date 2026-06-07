# Host Link - unified overview

End-to-end companion-app link, spanning **both firmwares**. This document is the
single cross-firmware view: how the pieces fit, who owns what, and where to look.
It deliberately does **not** repeat the per-file reference tables - those live in
the component READMEs, and the byte-level wire format lives in the protocol spec.

- Wire spec: [`protocol.md`](./protocol.md)
- SPI bridge (P4↔C5 transport this rides on): [`../spi_bridge/README.md`](../spi_bridge/README.md)
- P4 component reference: [`p4.md`](./p4.md) · in-tree: [`firmware_p4/.../host_link/README.md`](../../firmware_p4/components/Service/host_link/README.md)
- C5 component reference: [`c5.md`](./c5.md) · in-tree: [`firmware_c5/.../host_link/README.md`](../../firmware_c5/components/Service/host_link/README.md)

## The model

```
                          USB CDC-ACM (P4-native)
 ┌──────────────┐  ◄───────────────────────────────►  ┌─────────────┐
 │ Companion app│                                      │  ESP32-P4   │
 │   (PC/phone) │  ◄───────────────────────────────►  │  (the brain)│
 └──────────────┘     BLE      ┌─────────────┐  relay  └─────────────┘
                  ◄───────────►│  ESP32-C5   │◄────────────────┘ SPI bridge
                               │ (BLE radio) │
                               └─────────────┘
```

- **The P4 is the single brain.** It terminates the security envelope, dispatches
  every command (locally or relayed to the C5 over SPI), and owns SD/flash and
  device state. Identical behavior on both transports - one place for crypto.
- **USB** terminates on the P4 (CDC-ACM in the TinyUSB composite, alongside the
  BadUSB HID).
- **BLE** terminates on the **C5** (it owns the radio). The C5 is a transparent
  byte relay - it never parses companion payloads; all auth is on the P4.
- **One companion session at a time.** The first transport to attach owns the
  session; a second attach is rejected until it releases.

## Frame envelope (summary)

```
[MAGIC 'H''B'][VER][FLAGS][COUNTER u32][LEN u16][BODY][MAC 16 if FLAGS.auth]
BODY = [type][category][op][payload]
```

`category`/`op` reuse the `spi_protocol.h` ids (`SPI_CMD(cat, op)`) - one HAL
shared by app, P4 and C5. Types: `CMD`, `RESP`, `STREAM`, `LOG`, `HELLO`,
`HELLO_ACK`. Full field semantics: see the wire spec.

## Security (P4 only)

- PSK (32 B) in NVS, auto-generated on first boot. Provisioned out-of-band: QR +
  hex on the P4 pairing screen (Settings → PAIRING) or the `hostlink psk` console
  command.
- `HELLO`/`HELLO_ACK` handshake → per-direction HKDF keys (`a2d`/`d2a`) + counter
  reset. Per-frame HMAC-SHA256 (truncated 16 B, mbedTLS) verified before any body
  parse; monotonic counter rejects replays. Only `HELLO` is accepted unauthenticated.
- BLE bonding is "just works" (LE Secure Connections, no MITM) on top of the PSK
  envelope - the PSK is the real trust boundary.

## Module map

**P4 (`firmware_p4/components/Service/host_link/`)** - core + both transports +
all local handlers: framing/dispatch/session arbitration, USB CDC, BLE relay,
security, the P4 log tee, the C5 log relay, file ops, device state/settings/
console-exec, and the streaming/heartbeat proxy.

**C5 (`firmware_c5/components/Service/host_link/`)** - BLE GATT server (NimBLE
NUS-style), the chunking transport to/from the P4, and the C5 log tee.

New SPI ids backing all this live in `spi_protocol.h` under `SPI_CAT_HOST = 0x06`
(BLE relay) plus P4-local `SPI_CAT_SYSTEM` ops (`SYSTEM_LOG`, `FILE_*`,
`DEVICE_STATE`, settings, console-exec). Per-file detail: the component READMEs.

## Command routing (P4)

After auth, each `CMD` is routed by id: file ops → local; device-state/settings/
console-exec → local; `SPI_CAT_SESSION` (heartbeat/stop) → stream proxy (local,
**not** relayed - the P4 keeps heartbeating the C5 itself); session-start ops
(sniffer) → `spi_session`; everything else → relayed to the C5.

## Logs & two consoles

Both chips tee their `ESP_LOGx` (without losing the local dev console). P4 logs
are emitted directly; C5 logs stream to the P4 (`SPI_ID_SYSTEM_LOG`) and are
re-emitted. Each `LOG` frame carries a `source` byte (P4 / C5) so the app renders
two separate consoles. Console-exec output is delivered as console LOG frames.

## Toggles (NVS, default on)

| Setting | Off behavior |
|---------|--------------|
| `console_exec` | app can't run raw console lines (structured `CMD`s still work) |
| `log_over_ble` | no background logs over BLE; **USB always carries logs**; console-exec output always delivered |

## Boot order

**P4 (`kernel.c`):** `host_link_state_init` → `host_link_stream_init` →
`host_link_init` → `host_link_cdc_init` → `host_link_log_init` →
`host_link_c5log_init` → `host_link_ble_init` (BLE advertising starts on demand:
`hostlink ble on`).

**C5 (`kernel.c`):** `c5_log_init` right after `spi_bridge_slave_init`. The GATT
server is started on demand by the P4, not at boot.

## Phase status

All 8 phases (core+CDC, P4 log tee, security, BLE relay, C5 log forward, file ops,
device state+toggles+console-exec, streaming+heartbeat proxy) are **implemented
and build-validated on both firmwares**. See §15 of the wire spec for the
per-phase breakdown.

## Caveats (not yet hardware-tested)

- The transport layer is unexercised: the dev board's native USB pads are
  unsoldered and BLE hasn't been run. Everything is build-validated only.
- **NimBLE is single-owner**: host-link BLE, MeshCore and Meshtastic are mutually
  exclusive.
- **One `spi_session`**: the on-device UI sniffer and the companion sniffer are
  mutually exclusive (a start preempts the other).
- Device→app frames larger than the BLE MTU are split across notifications and
  reassembled by the app via `LEN`.
