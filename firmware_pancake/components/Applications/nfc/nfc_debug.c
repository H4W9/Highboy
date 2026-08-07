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
#include "nfc_debug.h"

#include "hb_nfc_spi.h"
#include "st25r3916_aat.h"
#include "st25r3916_core.h"
#include "st25r3916_reg.h"

static const char *TAG = "NFC_DEBUG";

hb_nfc_err_t nfc_debug_cw_on(void) {
  return st25r3916_core_field_on();
}

void nfc_debug_cw_off(void) {
  st25r3916_core_field_off();
}

void nfc_debug_dump_regs(void) {
  st25r3916_core_dump_regs();
}

hb_nfc_err_t nfc_debug_aat_sweep(void) {
  st25r3916_aat_result_t result;
  return st25r3916_aat_calibrate(&result);
}
