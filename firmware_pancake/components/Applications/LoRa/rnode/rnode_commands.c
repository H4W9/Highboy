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

#include "rnode_internal.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"

static const char *TAG = "RNODE_CMD";

static uint32_t read_be32(const uint8_t *p);
static uint16_t read_be16(const uint8_t *p);
static void write_be32(uint8_t *out, uint32_t v);

static void handle_detect(const uint8_t *payload, size_t len);
static void handle_fw_version(const uint8_t *payload, size_t len);
static void handle_platform(const uint8_t *payload, size_t len);
static void handle_mcu(const uint8_t *payload, size_t len);
static void handle_frequency(const uint8_t *payload, size_t len);
static void handle_bandwidth(const uint8_t *payload, size_t len);
static void handle_txpower(const uint8_t *payload, size_t len);
static void handle_sf(const uint8_t *payload, size_t len);
static void handle_cr(const uint8_t *payload, size_t len);
static void handle_radio_state(const uint8_t *payload, size_t len);
static void handle_data(const uint8_t *payload, size_t len);
static void handle_st_alock(const uint8_t *payload, size_t len);
static void handle_lt_alock(const uint8_t *payload, size_t len);
static void handle_blink(const uint8_t *payload, size_t len);
static void handle_random(const uint8_t *payload, size_t len);
static void handle_reset(const uint8_t *payload, size_t len);
static void handle_leave(const uint8_t *payload, size_t len);
static void handle_stat_chtm(const uint8_t *payload, size_t len);
static void handle_stat_phyprm(const uint8_t *payload, size_t len);
static void handle_stat_bat(const uint8_t *payload, size_t len);
static void handle_stat_temp(const uint8_t *payload, size_t len);
static void handle_stat_csma(const uint8_t *payload, size_t len);

void rnode_dispatch(const uint8_t *frame, size_t len) {
  if (frame == NULL || len == 0) {
    return;
  }
  uint8_t cmd = frame[0];
  const uint8_t *payload = (len > 1) ? &frame[1] : NULL;
  size_t plen = (len > 1) ? len - 1 : 0;

  switch (cmd) {
    case RNODE_CMD_DATA:
      handle_data(payload, plen);
      break;
    case RNODE_CMD_FREQUENCY:
      handle_frequency(payload, plen);
      break;
    case RNODE_CMD_BANDWIDTH:
      handle_bandwidth(payload, plen);
      break;
    case RNODE_CMD_TXPOWER:
      handle_txpower(payload, plen);
      break;
    case RNODE_CMD_SF:
      handle_sf(payload, plen);
      break;
    case RNODE_CMD_CR:
      handle_cr(payload, plen);
      break;
    case RNODE_CMD_RADIO_STATE:
      handle_radio_state(payload, plen);
      break;
    case RNODE_CMD_DETECT:
      handle_detect(payload, plen);
      break;
    case RNODE_CMD_LEAVE:
      handle_leave(payload, plen);
      break;
    case RNODE_CMD_ST_ALOCK:
      handle_st_alock(payload, plen);
      break;
    case RNODE_CMD_LT_ALOCK:
      handle_lt_alock(payload, plen);
      break;
    case RNODE_CMD_STAT_CHTM:
      handle_stat_chtm(payload, plen);
      break;
    case RNODE_CMD_STAT_PHYPRM:
      handle_stat_phyprm(payload, plen);
      break;
    case RNODE_CMD_STAT_BAT:
      handle_stat_bat(payload, plen);
      break;
    case RNODE_CMD_STAT_CSMA:
      handle_stat_csma(payload, plen);
      break;
    case RNODE_CMD_STAT_TEMP:
      handle_stat_temp(payload, plen);
      break;
    case RNODE_CMD_BLINK:
      handle_blink(payload, plen);
      break;
    case RNODE_CMD_RANDOM:
      handle_random(payload, plen);
      break;
    case RNODE_CMD_FW_VERSION:
      handle_fw_version(payload, plen);
      break;
    case RNODE_CMD_PLATFORM:
      handle_platform(payload, plen);
      break;
    case RNODE_CMD_MCU:
      handle_mcu(payload, plen);
      break;
    case RNODE_CMD_RESET:
      handle_reset(payload, plen);
      break;
    case RNODE_CMD_RADIO_LOCK:
      break;
    default:
      ESP_LOGW(TAG, "Unhandled CMD 0x%02X (%u bytes)", cmd, (unsigned)plen);
      break;
  }
}

static uint32_t read_be32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t read_be16(const uint8_t *p) {
  return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static void write_be32(uint8_t *out, uint32_t v) {
  out[0] = (uint8_t)((v >> 24) & 0xFF);
  out[1] = (uint8_t)((v >> 16) & 0xFF);
  out[2] = (uint8_t)((v >> 8) & 0xFF);
  out[3] = (uint8_t)(v & 0xFF);
}

static void handle_detect(const uint8_t *payload, size_t len) {
  if (len < 1 || payload[0] != RNODE_DETECT_PROBE) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint8_t resp = RNODE_CMD_DETECT_RESP;
  rnode_send_frame(RNODE_CMD_DETECT, &resp, 1);
}

static void handle_fw_version(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t out[2] = {RNODE_FW_VERSION_MAJOR, RNODE_FW_VERSION_MINOR};
  rnode_send_frame(RNODE_CMD_FW_VERSION, out, sizeof(out));
}

static void handle_platform(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t out = RNODE_PLATFORM_ESP32;
  rnode_send_frame(RNODE_CMD_PLATFORM, &out, 1);
}

static void handle_mcu(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t out = RNODE_MCU_ESP32;
  rnode_send_frame(RNODE_CMD_MCU, &out, 1);
}

static void handle_frequency(const uint8_t *payload, size_t len) {
  if (len < 4) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint32_t freq = read_be32(payload);
  rnode_cfg_mut()->freq_hz = freq;
  rnode_nvs_save_cfg();
  rnode_apply_radio_cfg();

  uint8_t out[4];
  write_be32(out, rnode_cfg_mut()->freq_hz);
  rnode_send_frame(RNODE_CMD_FREQUENCY, out, sizeof(out));
}

static void handle_bandwidth(const uint8_t *payload, size_t len) {
  if (len < 4) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint32_t bw = read_be32(payload);
  rnode_cfg_mut()->bw_hz = bw;
  rnode_nvs_save_cfg();
  rnode_apply_radio_cfg();

  uint8_t out[4];
  write_be32(out, rnode_cfg_mut()->bw_hz);
  rnode_send_frame(RNODE_CMD_BANDWIDTH, out, sizeof(out));
}

static void handle_txpower(const uint8_t *payload, size_t len) {
  if (len < 1) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  int8_t power = (int8_t)payload[0];
  rnode_cfg_mut()->tx_power_dbm = power;
  rnode_nvs_save_cfg();
  rnode_apply_radio_cfg();

  uint8_t out = (uint8_t)rnode_cfg_mut()->tx_power_dbm;
  rnode_send_frame(RNODE_CMD_TXPOWER, &out, 1);
}

static void handle_sf(const uint8_t *payload, size_t len) {
  if (len < 1) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint8_t sf = payload[0];
  if (sf < 5 || sf > 12) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  rnode_cfg_mut()->sf = sf;
  rnode_nvs_save_cfg();
  rnode_apply_radio_cfg();

  uint8_t out = rnode_cfg_mut()->sf;
  rnode_send_frame(RNODE_CMD_SF, &out, 1);
}

static void handle_cr(const uint8_t *payload, size_t len) {
  if (len < 1) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint8_t cr = payload[0];
  if (cr < 5 || cr > 8) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  rnode_cfg_mut()->cr = cr;
  rnode_nvs_save_cfg();
  rnode_apply_radio_cfg();

  uint8_t out = rnode_cfg_mut()->cr;
  rnode_send_frame(RNODE_CMD_CR, &out, 1);
}

static void handle_radio_state(const uint8_t *payload, size_t len) {
  rnode_radio_cfg_t *cfg = rnode_cfg_mut();
  if (len >= 1 && payload[0] != 0xFF) {
    bool is_on = (payload[0] != 0);
    if (rnode_set_radio_state(is_on) != ESP_OK) {
      rnode_send_error(RNODE_ERROR_INITRADIO);
      return;
    }
    cfg->is_radio_on = is_on;
  }
  uint8_t out = cfg->is_radio_on ? 0x01 : 0x00;
  rnode_send_frame(RNODE_CMD_RADIO_STATE, &out, 1);
}

static void handle_data(const uint8_t *payload, size_t len) {
  if (len == 0) {
    return;
  }
  if (rnode_airtime_is_blocked()) {
    rnode_send_error(RNODE_ERROR_QUEUE_FULL);
    return;
  }
  esp_err_t ret = rnode_radio_transmit(payload, len);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "TX failed: %s", esp_err_to_name(ret));
    rnode_send_error(RNODE_ERROR_TXFAILED);
    return;
  }
  rnode_stats_mut()->tx_count++;
}

static void handle_st_alock(const uint8_t *payload, size_t len) {
  if (len < 2) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint16_t st = read_be16(payload);
  rnode_airtime_set_limits(st, rnode_airtime_long());
  rnode_send_frame(RNODE_CMD_ST_ALOCK, payload, 2);
}

static void handle_lt_alock(const uint8_t *payload, size_t len) {
  if (len < 2) {
    rnode_send_error(RNODE_ERROR_INITRADIO);
    return;
  }
  uint16_t lt = read_be16(payload);
  rnode_airtime_set_limits(rnode_airtime_short(), lt);
  rnode_send_frame(RNODE_CMD_LT_ALOCK, payload, 2);
}

static void handle_blink(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  ESP_LOGI(TAG, "BLINK");
}

static void handle_random(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t r = (uint8_t)(esp_random() & 0xFF);
  rnode_send_frame(RNODE_CMD_RANDOM, &r, 1);
}

static void handle_reset(const uint8_t *payload, size_t len) {
  if (len < 1 || payload[0] != RNODE_RESET_MAGIC) {
    return;
  }
  ESP_LOGW(TAG, "Host requested reset");
  esp_restart();
}

static void handle_leave(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  ESP_LOGI(TAG, "Host went offline");
}

static void handle_stat_chtm(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t out[11] = {0};
  uint16_t st = rnode_airtime_short();
  uint16_t lt = rnode_airtime_long();
  out[0] = (uint8_t)((st >> 8) & 0xFF);
  out[1] = (uint8_t)(st & 0xFF);
  out[2] = (uint8_t)((lt >> 8) & 0xFF);
  out[3] = (uint8_t)(lt & 0xFF);
  rnode_send_frame(RNODE_CMD_STAT_CHTM, out, sizeof(out));
}

static void handle_stat_phyprm(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  const rnode_radio_cfg_t *cfg = rnode_get_radio_cfg();
  uint32_t symbol_time_us = 0;
  if (cfg->bw_hz > 0) {
    symbol_time_us = ((uint32_t)1U << cfg->sf) * 1000000U / cfg->bw_hz;
  }
  uint8_t out[12] = {0};
  out[0] = cfg->sf;
  out[1] = cfg->cr;
  write_be32(&out[2], cfg->bw_hz);
  write_be32(&out[6], symbol_time_us);
  out[10] = 8;
  out[11] = 0;
  rnode_send_frame(RNODE_CMD_STAT_PHYPRM, out, sizeof(out));
}

static void handle_stat_bat(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t out[2];
  out[0] = 0x01;
  out[1] = rnode_get_stats()->battery_pct;
  rnode_send_frame(RNODE_CMD_STAT_BAT, out, sizeof(out));
}

static void handle_stat_temp(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  int8_t temp_c = rnode_get_stats()->cpu_temp_c;
  uint8_t out = (uint8_t)(temp_c + RNODE_TEMP_OFFSET);
  rnode_send_frame(RNODE_CMD_STAT_TEMP, &out, 1);
}

static void handle_stat_csma(const uint8_t *payload, size_t len) {
  (void)payload;
  (void)len;
  uint8_t out[3] = {0, 0, 0};
  rnode_send_frame(RNODE_CMD_STAT_CSMA, out, sizeof(out));
}
