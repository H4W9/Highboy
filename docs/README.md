# Documentation hub

Aggregated, canonical copies of the project documentation, one directory per
component. Each component's in-tree README points back to its copy here.
Components that exist in both firmwares are split into `p4.md` / `c5.md`;
cross-firmware features keep their overview in the directory's `README.md`.

## Featured

- [host_link/](host_link/README.md) - companion app link: overview, [app implementation guide](host_link/app-guide.md), [protocol spec](host_link/protocol.md), per-firmware refs
- [spi_bridge/](spi_bridge/README.md) - P4<->C5 SPI bridge: architecture + per-firmware refs

## All components

| Component | Docs |
|-----------|------|
| `bad_usb` | [README.md](bad_usb/README.md)  |
| `bluetooth` | [README.md](bluetooth/README.md)  |
| `buttons_gpio` | [c5.md](buttons_gpio/c5.md) [p4.md](buttons_gpio/p4.md)  |
| `c5_flasher` | [README.md](c5_flasher/README.md)  |
| `cc1101` | [README.md](cc1101/README.md)  |
| `console` | [README.md](console/README.md)  |
| `dns_server` | [README.md](dns_server/README.md)  |
| `esp_now` | [README.md](esp_now/README.md)  |
| `espnow_chat` | [README.md](espnow_chat/README.md)  |
| `host_link` | [app-guide.md](host_link/app-guide.md) [c5.md](host_link/c5.md) [p4.md](host_link/p4.md) [protocol.md](host_link/protocol.md) [README.md](host_link/README.md)  |
| `http_server` | [README.md](http_server/README.md)  |
| `lvgl_port` | [README.md](lvgl_port/README.md)  |
| `ota` | [README.md](ota/README.md)  |
| `sd_card` | [c5.md](sd_card/c5.md) [p4.md](sd_card/p4.md)  |
| `spi` | [c5.md](spi/c5.md) [p4.md](spi/p4.md)  |
| `spi_bridge` | [c5.md](spi_bridge/c5.md) [p4.md](spi_bridge/p4.md) [README.md](spi_bridge/README.md)  |
| `st7789` | [README.md](st7789/README.md)  |
| `storage_api` | [c5.md](storage_api/c5.md) [p4.md](storage_api/p4.md)  |
| `storage_assets` | [c5.md](storage_assets/c5.md) [p4.md](storage_assets/p4.md)  |
| `storage_vfs` | [c5.md](storage_vfs/c5.md) [p4.md](storage_vfs/p4.md)  |
| `SubGhz` | [README.md](SubGhz/README.md)  |
| `tusb_desc` | [README.md](tusb_desc/README.md)  |
| `ui` | [README.md](ui/README.md)  |
| `wifi` | [c5.md](wifi/c5.md) [p4.md](wifi/p4.md)  |
