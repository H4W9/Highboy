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

#include "meshtastic_presets.h"

#include "esp_log.h"

#include "sx1262_regs.h"

#include "meshtastic_nvs.h"

static const char *TAG = "MESHTASTIC_PRESETS";

#define MT_PRESET_NVS_KEY "preset"

static const mt_preset_info_t PRESETS[] = {
    {MT_PRESET_LONG_FAST, "LONG_FAST", 11, 250000, 5, "Padrao, universal"},
    {MT_PRESET_LONG_SLOW, "LONG_SLOW", 12, 125000, 8, "Obsoleto"},
    {MT_PRESET_VERY_LONG_SLOW, "VERY_LONG_SLOW", 12, 125000, 8, "Obsoleto, requer TCXO"},
    {MT_PRESET_MEDIUM_SLOW, "MEDIUM_SLOW", 10, 250000, 5, "Alcance medio"},
    {MT_PRESET_MEDIUM_FAST, "MEDIUM_FAST", 9, 250000, 5, "Medio, mais rapido"},
    {MT_PRESET_SHORT_SLOW, "SHORT_SLOW", 8, 250000, 5, "Local"},
    {MT_PRESET_SHORT_FAST, "SHORT_FAST", 7, 250000, 5, "Local, rapido"},
    {MT_PRESET_LONG_MODERATE, "LONG_MODERATE", 11, 125000, 8, "Longo, moderado"},
    {MT_PRESET_SHORT_TURBO, "SHORT_TURBO", 7, 500000, 5, "Mais rapido, BW 500 kHz"},
    {MT_PRESET_LONG_TURBO, "LONG_TURBO", 11, 500000, 8, "Longo, BW 500 kHz"},
};
#define PRESETS_COUNT (sizeof(PRESETS) / sizeof(PRESETS[0]))

const mt_preset_info_t *mt_preset_info(mt_preset_t id) {
  for (size_t i = 0; i < PRESETS_COUNT; i++) {
    if (PRESETS[i].id == id)
      return &PRESETS[i];
  }
  return NULL;
}

int mt_preset_count(void) {
  return (int)PRESETS_COUNT;
}

mt_preset_t mt_preset_current(void) {
  uint32_t value = mt_nvs_get_u32(MT_PRESET_NVS_KEY, (uint32_t)MT_PRESET_LONG_FAST);
  return (mt_preset_t)value;
}

void mt_preset_set(mt_preset_t id) {
  const mt_preset_info_t *info = mt_preset_info(id);
  if (info == NULL) {
    ESP_LOGW(TAG, "Preset invalid: %d", (int)id);
    return;
  }
  mt_nvs_set_u32(MT_PRESET_NVS_KEY, (uint32_t)id);
  ESP_LOGI(TAG,
           "Preset applied: %s (SF%u BW%lu CR4/%u)",
           info->name,
           info->sf,
           (unsigned long)info->bw_hz,
           info->cr);
}

uint8_t mt_preset_bw_to_sx1262(uint32_t bw_hz) {
  if (bw_hz <= 7812)
    return SX1262_LORA_BW_7;
  if (bw_hz <= 10420)
    return SX1262_LORA_BW_10;
  if (bw_hz <= 15630)
    return SX1262_LORA_BW_15;
  if (bw_hz <= 20830)
    return SX1262_LORA_BW_20;
  if (bw_hz <= 31250)
    return SX1262_LORA_BW_31;
  if (bw_hz <= 41670)
    return SX1262_LORA_BW_41;
  if (bw_hz <= 62500)
    return SX1262_LORA_BW_62;
  if (bw_hz <= 125000)
    return SX1262_LORA_BW_125;
  if (bw_hz <= 250000)
    return SX1262_LORA_BW_250;
  return SX1262_LORA_BW_500;
}
