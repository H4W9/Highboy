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

#ifndef MESHTASTIC_PRESETS_H
#define MESHTASTIC_PRESETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief LoRa modem preset identifiers.
 *
 * Values match LoRaConfig.ModemPreset in the official Meshtastic
 * protobuf definition (config.proto).
 */
typedef enum {
  MT_PRESET_LONG_FAST = 0,
  MT_PRESET_LONG_SLOW = 1,
  MT_PRESET_VERY_LONG_SLOW = 2,
  MT_PRESET_MEDIUM_SLOW = 3,
  MT_PRESET_MEDIUM_FAST = 4,
  MT_PRESET_SHORT_SLOW = 5,
  MT_PRESET_SHORT_FAST = 6,
  MT_PRESET_LONG_MODERATE = 7,
  MT_PRESET_SHORT_TURBO = 8,
  MT_PRESET_LONG_TURBO = 9,
  MT_PRESET_COUNT
} mt_preset_t;

/**
 * @brief Static information about a modem preset.
 */
typedef struct {
  mt_preset_t id;
  const char *name;
  uint8_t sf;
  uint32_t bw_hz;
  uint8_t cr;
  const char *description;
} mt_preset_info_t;

/**
 * @brief Look up the info table entry for a preset id.
 *
 * @param id  Preset identifier.
 * @return Pointer to static info table entry, or NULL if id is invalid.
 */
const mt_preset_info_t *mt_preset_info(mt_preset_t id);

/**
 * @brief Total number of modem presets supported.
 */
int mt_preset_count(void);

/**
 * @brief Return the preset currently persisted in NVS.
 *
 * Falls back to MT_PRESET_LONG_FAST if NVS is empty.
 */
mt_preset_t mt_preset_current(void);

/**
 * @brief Persist the given preset to NVS.
 */
void mt_preset_set(mt_preset_t id);

/**
 * @brief Map bandwidth in Hz to the SX1262 driver's BW enum value.
 *
 * @param bw_hz  Bandwidth in Hz.
 * @return SX1262_LORA_BW_* value that matches (rounds up to next supported).
 */
uint8_t mt_preset_bw_to_sx1262(uint32_t bw_hz);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_PRESETS_H
