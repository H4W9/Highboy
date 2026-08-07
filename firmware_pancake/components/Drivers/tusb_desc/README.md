# TinyUSB Descriptors (HID Composite)

This component defines the USB descriptors required to enumerate the ESP32-P4 as a USB HID Composite Device (Keyboard + Mouse) and provides the initialization routine for the TinyUSB driver.

## Overview

- **Location:** `components/Drivers/tusb_desc/`
- **Header:** `include/tusb_desc.h`
- **Dependencies:** `tinyusb`, `esp_tinyusb`, `driver/gpio`
- **USB Port:** High Speed (ESP32-P4)

## USB Descriptors

### Device Descriptor

| Field | Value |
|-------|-------|
| USB Version | 2.0 |
| Vendor ID | `0xCAFE` |
| Product ID | `0x4001` |
| Device Class | Defined at interface level |
| Configurations | 1 |

### Configuration Descriptor

| Field | Value |
|-------|-------|
| Interfaces | 1 (HID) |
| Max Power | 100 mA |
| Attributes | Remote Wakeup |

### HID Report Descriptor

Single HID interface with two reports using Report IDs:

| Report ID | Type | Usage |
|-----------|------|-------|
| 1 | Keyboard | Generic Desktop Keyboard |
| 2 | Mouse | Generic Desktop Mouse (buttons + XY + wheel) |

### String Descriptors

| Index | Value |
|-------|-------|
| 0 | Language ID (English US) |
| 1 | Manufacturer: "HighCode" |
| 2 | Product: "BadUSB Device" |
| 3 | Serial: "123456" |

## API Reference

### `busb_init`
```c
esp_err_t busb_init(void);
```
Initializes the TinyUSB driver with the defined descriptors.
1. Installs the GPIO ISR service (required for ESP32-P4 High Speed USB).
2. Configures device, configuration, and HID report descriptors.
3. Installs the TinyUSB driver on the High Speed port.

Must be called before any HID report transmission.

## TinyUSB Callbacks

The component implements the required TinyUSB callbacks to serve descriptors to the USB host:

| Callback | Purpose |
|----------|---------|
| `tud_descriptor_device_cb` | Returns the device descriptor |
| `tud_descriptor_configuration_cb` | Returns the configuration descriptor |
| `tud_descriptor_string_cb` | Returns string descriptors (manufacturer, product, serial) |
| `tud_hid_descriptor_report_cb` | Returns the HID report descriptor |
| `tud_hid_get_report_cb` | Handles GET_REPORT requests (stub) |
| `tud_hid_set_report_cb` | Handles SET_REPORT requests (stub) |
