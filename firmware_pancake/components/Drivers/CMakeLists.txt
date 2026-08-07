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

file(GLOB_RECURSE ST25R3916_SRCS "st25r3916/*.c")
file(GLOB_RECURSE YS_RFID2_SRCS "ys_rfid2/*.c")
file(GLOB_RECURSE SX1262_SRCS "sx1262/*.c")


idf_component_register(SRCS
  "bq25896/bq25896.c"
  "cc1101/cc1101.c"
  "st7789/st7789.c"
  "ft6336/ft6336.c"
  "buzzer/buzzer.c"
  "led/led_control.c"
  "spi/spi.c"
  "i2c_init/i2c_init.c"
  "buttons_gpio/buttons_gpio.c"
  "spi_bridge_phy/spi_bridge_phy.c"
  ${ST25R3916_SRCS}
  ${YS_RFID2_SRCS}
  ${SX1262_SRCS}

  INCLUDE_DIRS
  "bq25896/include"
  "cc1101/include"
  "st7789/include"
  "ft6336/include"
  "buzzer/include"
  "led/include"
  "pins/include"
  "spi/include"
  "i2c_init/include"
  "buttons_gpio/include"
  "spi_bridge_phy/include"
  "st25r3916/include"
  "st25r3916/hal/include"
  "ys_rfid2/include"
  "ys_rfid2/hal/include"
  "sx1262/include"

  # Pancake (ESP32-C5): no USB-OTG, so tinyusb/tusb_desc are dropped and the
  # ST7796S panel driver replaces the built-in esp_lcd st7789 vendor init.
  REQUIRES
  driver
  Service
  esp_lcd
  PRIV_REQUIRES
  "espressif__led_strip")
