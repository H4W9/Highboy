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

#include "ir_ac.h"

#include "esp_log.h"

#include "ir.h"
#include "ir_ac_coolix.h"
#include "ir_ac_gree.h"

static const char *TAG = "IR_AC";

const char *ir_ac_protocol_name(ir_ac_protocol_t proto) {
  switch (proto) {
    case IR_AC_PROTO_COOLIX:
      return "COOLIX";
    case IR_AC_PROTO_GREE:
      return "GREE";
    default:
      return "UNKNOWN";
  }
}

uint32_t ir_ac_carrier_freq(ir_ac_protocol_t proto) {
  switch (proto) {
    case IR_AC_PROTO_COOLIX:
      return COOLIX_CARRIER_HZ;
    case IR_AC_PROTO_GREE:
      return GREE_CARRIER_HZ;
    default:
      return COOLIX_CARRIER_HZ;
  }
}

size_t ir_ac_encode(const ir_ac_state_t *state, rmt_symbol_word_t *symbols, size_t max) {
  if (state == NULL || symbols == NULL || max == 0)
    return 0;

  switch (state->protocol) {
    case IR_AC_PROTO_COOLIX:
      return ir_ac_coolix_encode(state, symbols, max);
    case IR_AC_PROTO_GREE:
      return ir_ac_gree_encode(state, symbols, max);
    default:
      ESP_LOGW(TAG, "Encode called with unknown AC protocol: %d", (int)state->protocol);
      return 0;
  }
}

esp_err_t ir_ac_send(const ir_ac_state_t *state) {
  if (state == NULL)
    return ESP_ERR_INVALID_ARG;

  rmt_symbol_word_t symbols[IR_RMT_MEM_SYMBOLS];
  size_t count = ir_ac_encode(state, symbols, IR_RMT_MEM_SYMBOLS);
  if (count == 0)
    return ESP_ERR_INVALID_ARG;

  return ir_send_raw(symbols, count, ir_ac_carrier_freq(state->protocol));
}
