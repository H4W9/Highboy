# Copyright (c) 2025 HIGH CODE LLC
#
# TentacleOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# TentacleOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with TentacleOS. If not, see <https://www.gnu.org/licenses/>.

file(GLOB_RECURSE SPI_BRIDGE_SRCS "spi_bridge/*.c")
file(GLOB_RECURSE SD_CARD_SRCS "sd_card/*.c")
file(GLOB_RECURSE MESHTASTIC_SRCS "meshtastic/*.c")
file(GLOB_RECURSE MESHCORE_SRCS "meshcore/*.c")


idf_component_register(SRCS 
  "wifi/wifi_service.c"
  "wifi/mac_vendor.c"
  "http_server/http_server_service.c"
  "dns_server/dns_server.c"
  "bluetooth/bluetooth_service.c"

  "storage_api/storage_impl.c"
  "storage_api/storage_init.c"
  "storage_api/storage_read.c"
  "storage_api/storage_write.c"
  "storage_api/storage_mkdir.c"
  "storage_vfs/vfs_auto.c"
  "storage_vfs/vfs_core.c"
  "storage_vfs/vfs_littlefs.c"
  "storage_vfs/vfs_sdcard.c"
  "storage_assets/storage_assets.c"

  "esp_now/service_esp_now.c"
  ${SPI_BRIDGE_SRCS}
  ${SD_CARD_SRCS}
  ${MESHTASTIC_SRCS}
  ${MESHCORE_SRCS}
  INCLUDE_DIRS 
  "wifi/include"
  "http_server/include"
  "dns_server/include"
  "bluetooth/include"
  "storage_api/include"
  "storage_vfs/include"
  "storage_assets/include"
  "esp_now/include"
  "spi_bridge/include"
  "sd_card/include"
  "meshtastic/include"
  "meshcore/include"




  REQUIRES 
    sdmmc
    driver  
    vfs
    fatfs
    esp_timer
    esp_common
    esp_netif
    esp_app_format
    Drivers
    Applications
    esp_http_server
    nvs_flash
    lvgl
    littlefs
    cjson
    mdns
)
