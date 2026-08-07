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

#include "mt_mod_admin.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "meshtastic_channels.h"
#include "meshtastic_mesh.h"
#include "meshtastic_nodedb.h"
#include "meshtastic_nvs.h"
#include "meshtastic_pki.h"

#include "meshtastic_roles.h"
#include "mt_mod_keyverify.h"
#include "mt_mod_nodeinfo.h"
#include "mt_mod_position.h"

static const char *TAG = "MT_MOD_ADMIN";

#define MT_ADMIN_PASSKEY_TTL_SECS  300
#define MT_ADMIN_PASSKEY_SIZE      8
#define MT_ADMIN_HOP_LIMIT         3
#define MT_ADMIN_HW_MODEL_PRIVATE  255
#define MT_ADMIN_ROLE_REPEATER     4
#define MT_ADMIN_FIRST_SET_VARIANT 32

#define MT_ADMIN_VARIANT_GET_CHANNEL_REQ         1
#define MT_ADMIN_VARIANT_GET_OWNER_REQ           3
#define MT_ADMIN_VARIANT_GET_CONFIG_REQ          5
#define MT_ADMIN_VARIANT_GET_DEVICE_METADATA_REQ 12

#define MT_ADMIN_VARIANT_GET_CHANNEL_RESP         2
#define MT_ADMIN_VARIANT_GET_OWNER_RESP           4
#define MT_ADMIN_VARIANT_GET_CONFIG_RESP          6
#define MT_ADMIN_VARIANT_GET_DEVICE_METADATA_RESP 13

#define MT_ADMIN_VARIANT_SET_OWNER          32
#define MT_ADMIN_VARIANT_SET_CHANNEL        33
#define MT_ADMIN_VARIANT_SET_CONFIG         34
#define MT_ADMIN_VARIANT_SET_FAVORITE       39
#define MT_ADMIN_VARIANT_REMOVE_FAVORITE    40
#define MT_ADMIN_VARIANT_SET_FIXED_POSITION 41
#define MT_ADMIN_VARIANT_REMOVE_FIXED_POS   42
#define MT_ADMIN_VARIANT_SET_TIME_ONLY      43
#define MT_ADMIN_VARIANT_BEGIN_EDIT         64
#define MT_ADMIN_VARIANT_COMMIT_EDIT        65

#define MT_ADMIN_VARIANT_KEY_VERIFICATION 67
#define MT_ADMIN_VARIANT_SESSION_PASSKEY  101

#define MT_ADMIN_MAX_KEYS     3
#define MT_ADMIN_KEY_LEN      32
#define MT_ADMIN_KEYS_NVS_KEY "admin_keys"

static uint8_t s_session_passkey[MT_ADMIN_PASSKEY_SIZE];
static uint32_t s_passkey_set_s = 0;
static uint32_t s_node_num = 0;
static bool s_is_edit_transaction_open = false;
static uint8_t s_admin_keys[MT_ADMIN_MAX_KEYS][MT_ADMIN_KEY_LEN];

static uint16_t enc_varint(uint8_t *buf, uint64_t value) {
  uint16_t pos = 0;
  do {
    uint8_t byte = value & 0x7F;
    value >>= 7;
    if (value)
      byte |= 0x80;
    buf[pos++] = byte;
  } while (value);
  return pos;
}

static uint16_t enc_tag(uint8_t *buf, uint32_t field_num, uint8_t wire_type) {
  return enc_varint(buf, ((uint64_t)field_num << 3) | (wire_type & 0x07));
}

static uint16_t enc_field_varint(uint8_t *buf, uint32_t field_num, uint64_t value) {
  uint16_t pos = enc_tag(buf, field_num, 0);
  pos += enc_varint(&buf[pos], value);
  return pos;
}

static uint16_t
enc_field_bytes(uint8_t *buf, uint32_t field_num, const uint8_t *data, uint16_t len) {
  uint16_t pos = enc_tag(buf, field_num, 2);
  pos += enc_varint(&buf[pos], len);
  if (len > 0 && data != NULL) {
    memcpy(&buf[pos], data, len);
  }
  return pos + len;
}

static uint16_t enc_field_fixed32(uint8_t *buf, uint32_t field_num, uint32_t value) {
  uint16_t pos = enc_tag(buf, field_num, 5);
  buf[pos++] = (uint8_t)(value & 0xFF);
  buf[pos++] = (uint8_t)((value >> 8) & 0xFF);
  buf[pos++] = (uint8_t)((value >> 16) & 0xFF);
  buf[pos++] = (uint8_t)((value >> 24) & 0xFF);
  return pos;
}

static uint64_t dec_varint(const uint8_t *buf, uint16_t max_len, uint16_t *out_used) {
  uint64_t value = 0;
  uint16_t shift = 0;
  uint16_t i = 0;
  while (i < max_len && i < 10) {
    uint8_t byte = buf[i++];
    value |= ((uint64_t)(byte & 0x7F)) << shift;
    if (!(byte & 0x80))
      break;
    shift += 7;
  }
  if (out_used != NULL)
    *out_used = i;
  return value;
}

static void regen_passkey(uint32_t now_s) {
  for (int i = 0; i < MT_ADMIN_PASSKEY_SIZE; i++) {
    s_session_passkey[i] = (uint8_t)(esp_random() & 0xFF);
  }
  s_passkey_set_s = now_s;
}

static bool is_passkey_valid(const uint8_t *provided, uint16_t provided_len) {
  uint32_t now_s = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
  if (now_s - s_passkey_set_s > MT_ADMIN_PASSKEY_TTL_SECS)
    return false;
  if (provided_len != MT_ADMIN_PASSKEY_SIZE)
    return false;
  return memcmp(provided, s_session_passkey, MT_ADMIN_PASSKEY_SIZE) == 0;
}

static bool admin_keys_any_set(void) {
  for (int i = 0; i < MT_ADMIN_MAX_KEYS; i++) {
    for (int k = 0; k < MT_ADMIN_KEY_LEN; k++) {
      if (s_admin_keys[i][k] != 0)
        return true;
    }
  }
  return false;
}

static bool is_sender_admin_authorized(uint32_t from_node) {
  if (!admin_keys_any_set())
    return true;

  const mt_node_entry_t *e = mt_nodedb_get(from_node);
  if (e == NULL || !e->has_public_key)
    return false;

  for (int i = 0; i < MT_ADMIN_MAX_KEYS; i++) {
    bool slot_zero = true;
    for (int k = 0; k < MT_ADMIN_KEY_LEN; k++) {
      if (s_admin_keys[i][k] != 0) {
        slot_zero = false;
        break;
      }
    }
    if (slot_zero)
      continue;
    if (memcmp(s_admin_keys[i], e->public_key, MT_ADMIN_KEY_LEN) == 0) {
      return true;
    }
  }
  return false;
}

static void admin_keys_load(void) {
  memset(s_admin_keys, 0, sizeof(s_admin_keys));
  mt_nvs_get_blob(MT_ADMIN_KEYS_NVS_KEY, s_admin_keys, sizeof(s_admin_keys));
}

static void admin_keys_save(void) {
  mt_nvs_set_blob(MT_ADMIN_KEYS_NVS_KEY, s_admin_keys, sizeof(s_admin_keys));
}

static void admin_keys_set_from_security_config(const uint8_t *security_bytes, uint16_t len) {
  uint8_t new_keys[MT_ADMIN_MAX_KEYS][MT_ADMIN_KEY_LEN] = {{0}};
  uint8_t count = 0;

  uint16_t i = 0;
  while (i < len && count < MT_ADMIN_MAX_KEYS) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&security_bytes[i], len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 2) {
      uint16_t lused = 0;
      uint32_t length = (uint32_t)dec_varint(&security_bytes[i], len - i, &lused);
      i += lused;
      if (i + length > len)
        break;
      if (field == 3 && length == MT_ADMIN_KEY_LEN) {
        memcpy(new_keys[count++], &security_bytes[i], MT_ADMIN_KEY_LEN);
      }
      i += length;
    } else if (wire_type == 0) {
      uint16_t vused = 0;
      dec_varint(&security_bytes[i], len - i, &vused);
      i += vused;
    } else {
      break;
    }
  }

  memcpy(s_admin_keys, new_keys, sizeof(s_admin_keys));
  admin_keys_save();
  ESP_LOGI(TAG, "admin_keys updated: %u slot(s) configured", count);
}

static uint16_t build_user(uint8_t *out) {
  uint16_t pos = 0;
  char id_str[16];
  snprintf(id_str, sizeof(id_str), "!%08lx", (unsigned long)s_node_num);
  pos += enc_field_bytes(&out[pos], 1, (const uint8_t *)id_str, (uint16_t)strlen(id_str));
  const char *long_name = mt_mod_nodeinfo_get_long();
  const char *short_name = mt_mod_nodeinfo_get_short();
  pos += enc_field_bytes(&out[pos], 2, (const uint8_t *)long_name, (uint16_t)strlen(long_name));
  pos += enc_field_bytes(&out[pos], 3, (const uint8_t *)short_name, (uint16_t)strlen(short_name));
  pos += enc_field_varint(&out[pos], 5, MT_ADMIN_HW_MODEL_PRIVATE);
  return pos;
}

static uint16_t wrap_admin(uint8_t *out,
                           uint8_t variant_field,
                           const uint8_t *variant_data,
                           uint16_t variant_len,
                           bool include_passkey,
                           uint32_t now_s) {
  uint16_t pos = 0;
  pos += enc_field_bytes(&out[pos], variant_field, variant_data, variant_len);
  if (include_passkey) {
    regen_passkey(now_s);
    pos += enc_field_bytes(
        &out[pos], MT_ADMIN_VARIANT_SESSION_PASSKEY, s_session_passkey, MT_ADMIN_PASSKEY_SIZE);
  }
  return pos;
}

static void handle_get_owner(const mt_packet_meta_t *meta, uint32_t now_s) {
  uint8_t user[96];
  uint16_t ulen = build_user(user);

  uint8_t admin[128];
  uint16_t alen = wrap_admin(admin, MT_ADMIN_VARIANT_GET_OWNER_RESP, user, ulen, true, now_s);

  ESP_LOGI(TAG, "get_owner -> 0x%08lX", (unsigned long)meta->from);
  meshtastic_mesh_send_data(meta->from,
                            meta->channel,
                            MT_ADMIN_HOP_LIMIT,
                            MT_PORT_ADMIN,
                            admin,
                            alen,
                            meta->id,
                            false,
                            false);
}

static void
handle_set_owner(const mt_packet_meta_t *meta, const uint8_t *user_bytes, uint16_t user_len) {
  (void)meta;
  char long_name[32] = {0};
  char short_name[8] = {0};

  uint16_t i = 0;
  while (i < user_len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&user_bytes[i], user_len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 2) {
      uint16_t len_used = 0;
      uint32_t length = (uint32_t)dec_varint(&user_bytes[i], user_len - i, &len_used);
      i += len_used;
      if (i + length > user_len)
        break;
      if (field == 2) {
        uint32_t copy = length < sizeof(long_name) - 1 ? length : sizeof(long_name) - 1;
        memcpy(long_name, &user_bytes[i], copy);
        long_name[copy] = 0;
      } else if (field == 3) {
        uint32_t copy = length < sizeof(short_name) - 1 ? length : sizeof(short_name) - 1;
        memcpy(short_name, &user_bytes[i], copy);
        short_name[copy] = 0;
      }
      i += length;
    } else if (wire_type == 0) {
      uint16_t vused = 0;
      dec_varint(&user_bytes[i], user_len - i, &vused);
      i += vused;
    } else {
      break;
    }
  }

  mt_mod_nodeinfo_set_name(long_name[0] ? long_name : NULL, short_name[0] ? short_name : NULL);
  ESP_LOGI(TAG, "set_owner - '%s' / '%s'", long_name, short_name);
}

static void handle_get_channel(const mt_packet_meta_t *meta, uint8_t idx, uint32_t now_s) {
  const mt_channel_t *ch = mt_channel_get(idx);

  uint8_t channel[160];
  uint16_t cl = 0;
  cl += enc_field_varint(&channel[cl], 1, idx);

  if (ch != NULL && ch->is_used) {
    uint8_t settings[96];
    uint16_t sl = 0;
    sl += enc_field_bytes(&settings[sl], 2, ch->psk, MT_PSK_SIZE);
    size_t name_len = strlen(ch->name);
    if (name_len > 0) {
      sl += enc_field_bytes(&settings[sl], 3, (const uint8_t *)ch->name, (uint16_t)name_len);
    }
    if (ch->id != 0) {
      sl += enc_field_fixed32(&settings[sl], 4, ch->id);
    }

    cl += enc_field_bytes(&channel[cl], 2, settings, sl);
    cl += enc_field_varint(&channel[cl], 3, (uint64_t)ch->role);
  } else {
    cl += enc_field_varint(&channel[cl], 3, 0);
  }

  uint8_t admin[192];
  uint16_t alen = wrap_admin(admin, MT_ADMIN_VARIANT_GET_CHANNEL_RESP, channel, cl, true, now_s);

  ESP_LOGI(TAG,
           "get_channel[%u] role=%d -> 0x%08lX",
           idx,
           ch ? (int)ch->role : 0,
           (unsigned long)meta->from);
  meshtastic_mesh_send_data(meta->from,
                            meta->channel,
                            MT_ADMIN_HOP_LIMIT,
                            MT_PORT_ADMIN,
                            admin,
                            alen,
                            meta->id,
                            false,
                            false);
}

static void handle_set_channel(const uint8_t *channel_bytes, uint16_t len) {
  int32_t index = 0;
  uint8_t role = MT_CH_DISABLED;
  bool has_role = false;
  const uint8_t *settings_ptr = NULL;
  uint16_t settings_len = 0;

  uint16_t i = 0;
  while (i < len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&channel_bytes[i], len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 0) {
      uint16_t vused = 0;
      uint64_t value = dec_varint(&channel_bytes[i], len - i, &vused);
      i += vused;
      if (field == 1)
        index = (int32_t)value;
      else if (field == 3) {
        role = (uint8_t)value;
        has_role = true;
      }
    } else if (wire_type == 2) {
      uint16_t lused = 0;
      uint32_t length = (uint32_t)dec_varint(&channel_bytes[i], len - i, &lused);
      i += lused;
      if (i + length > len)
        break;
      if (field == 2) {
        settings_ptr = &channel_bytes[i];
        settings_len = (uint16_t)length;
      }
      i += length;
    } else {
      break;
    }
  }

  if (index < 0 || index >= MT_MAX_CHANNELS) {
    ESP_LOGW(TAG, "set_channel: index %ld out of range [0..%d)", (long)index, MT_MAX_CHANNELS);
    return;
  }
  if (!has_role) {
    ESP_LOGW(TAG, "set_channel[%ld]: missing role, ignoring", (long)index);
    return;
  }
  if (role == MT_CH_DISABLED) {
    mt_channel_disable((uint8_t)index);
    ESP_LOGI(TAG, "set_channel[%ld] disabled", (long)index);
    return;
  }

  const uint8_t *psk_ptr = NULL;
  uint16_t psk_len = 0;
  char name[32] = {0};
  bool has_name = false;
  uint32_t ch_id = 0;
  bool has_id = false;

  i = 0;
  while (settings_ptr != NULL && i < settings_len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&settings_ptr[i], settings_len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 2) {
      uint16_t lused = 0;
      uint32_t length = (uint32_t)dec_varint(&settings_ptr[i], settings_len - i, &lused);
      i += lused;
      if (i + length > settings_len)
        break;
      if (field == 2) {
        psk_ptr = &settings_ptr[i];
        psk_len = (uint16_t)length;
      } else if (field == 3) {
        uint16_t copy = length < sizeof(name) - 1 ? (uint16_t)length : sizeof(name) - 1;
        memcpy(name, &settings_ptr[i], copy);
        name[copy] = 0;
        has_name = true;
      }
      i += length;
    } else if (wire_type == 0) {
      uint16_t vused = 0;
      dec_varint(&settings_ptr[i], settings_len - i, &vused);
      i += vused;
    } else if (wire_type == 5) {
      if (i + 4 > settings_len)
        break;
      if (field == 4) {
        ch_id = ((uint32_t)settings_ptr[i]) | ((uint32_t)settings_ptr[i + 1] << 8) |
                ((uint32_t)settings_ptr[i + 2] << 16) | ((uint32_t)settings_ptr[i + 3] << 24);
        has_id = true;
      }
      i += 4;
    } else {
      break;
    }
  }

  const uint8_t *psk_to_save = NULL;
  if (psk_ptr != NULL && psk_len == MT_PSK_SIZE) {
    psk_to_save = psk_ptr;
  } else if (psk_ptr != NULL && psk_len > 0) {
    ESP_LOGW(TAG,
             "set_channel[%ld]: PSK %u bytes - only %d supported, keeping current PSK",
             (long)index,
             psk_len,
             MT_PSK_SIZE);
  }

  mt_channel_set((uint8_t)index, has_name ? name : NULL, psk_to_save, (mt_channel_role_t)role);
  if (has_id) {
    mt_channel_set_id((uint8_t)index, ch_id);
  }
  ESP_LOGI(TAG,
           "set_channel[%ld] role=%u name='%s' psk=%s id=0x%08lx",
           (long)index,
           role,
           has_name ? name : "(kept)",
           psk_to_save ? "updated" : "mantido",
           (unsigned long)(has_id ? ch_id : 0));
}

static void handle_get_device_metadata(const mt_packet_meta_t *meta, uint32_t now_s) {
  uint8_t metadata[96];
  uint16_t mlen = 0;
  const char *version = "2.7.0-highboy";
  mlen += enc_field_bytes(&metadata[mlen], 1, (const uint8_t *)version, (uint16_t)strlen(version));
  mlen += enc_field_varint(&metadata[mlen], 3, 4);
  mlen += enc_field_varint(&metadata[mlen], 4, 1);
  mlen += enc_field_varint(&metadata[mlen], 8, MT_ADMIN_HW_MODEL_PRIVATE);
  mlen += enc_field_varint(&metadata[mlen], 9, MT_ADMIN_ROLE_REPEATER);

  uint8_t admin[128];
  uint16_t alen =
      wrap_admin(admin, MT_ADMIN_VARIANT_GET_DEVICE_METADATA_RESP, metadata, mlen, true, now_s);

  ESP_LOGI(TAG, "get_device_metadata -> 0x%08lX", (unsigned long)meta->from);
  meshtastic_mesh_send_data(meta->from,
                            meta->channel,
                            MT_ADMIN_HOP_LIMIT,
                            MT_PORT_ADMIN,
                            admin,
                            alen,
                            meta->id,
                            false,
                            false);
}

static uint16_t build_config_for_type(uint8_t *out, uint8_t config_type) {
  uint8_t oneof_field = (uint8_t)(config_type + 1);

  if (config_type == 5) {
    uint8_t lora[48];
    uint16_t ll = 0;
    ll += enc_field_varint(&lora[ll], 1, 1);
    ll += enc_field_varint(&lora[ll], 2, 0);
    ll += enc_field_varint(&lora[ll], 3, 250);
    ll += enc_field_varint(&lora[ll], 4, 11);
    ll += enc_field_varint(&lora[ll], 5, 5);
    ll += enc_field_varint(&lora[ll], 7, 1);
    ll += enc_field_varint(&lora[ll], 8, 3);
    ll += enc_field_varint(&lora[ll], 9, 1);
    ll += enc_field_varint(&lora[ll], 10, 22);
    ll += enc_field_varint(&lora[ll], 11, 0);
    ll += enc_field_varint(&lora[ll], 13, 1);
    return enc_field_bytes(out, oneof_field, lora, ll);
  }

  if (config_type == 7) {
    uint8_t sec[192];
    uint16_t sl = 0;
    sl += enc_field_bytes(&sec[sl], 1, mt_pki_get_pubkey(), MT_PKI_KEY_LEN);
    sl += enc_field_bytes(&sec[sl], 2, mt_pki_get_privkey(), MT_PKI_KEY_LEN);
    for (int k = 0; k < MT_ADMIN_MAX_KEYS; k++) {
      bool zero = true;
      for (int b = 0; b < MT_ADMIN_KEY_LEN; b++) {
        if (s_admin_keys[k][b] != 0) {
          zero = false;
          break;
        }
      }
      if (!zero) {
        sl += enc_field_bytes(&sec[sl], 3, s_admin_keys[k], MT_ADMIN_KEY_LEN);
      }
    }
    return enc_field_bytes(out, oneof_field, sec, sl);
  }

  out[0] = (uint8_t)((oneof_field << 3) | 2);
  out[1] = 0;
  return 2;
}

static void parse_device_config(const uint8_t *buf, uint16_t len) {
  uint16_t i = 0;
  while (i < len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&buf[i], len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 0) {
      uint16_t vused = 0;
      uint64_t value = dec_varint(&buf[i], len - i, &vused);
      i += vused;
      if (field == 1) {
        mt_role_t new_role = (mt_role_t)value;
        if (new_role < MT_ROLE_COUNT) {
          mt_role_set(new_role);
          ESP_LOGI(TAG, "DeviceConfig.role -> %s", mt_role_name(new_role));
        } else {
          ESP_LOGW(TAG, "DeviceConfig.role invalid: %llu", (unsigned long long)value);
        }
      } else if (field == 6) {
        mt_rebr_mode_t new_mode = (mt_rebr_mode_t)value;
        if (new_mode < MT_REBR_COUNT) {
          mt_rebr_mode_set(new_mode);
          ESP_LOGI(TAG, "DeviceConfig.rebroadcast_mode -> %d", (int)new_mode);
        }
      }
    } else if (wire_type == 2) {
      uint16_t lused = 0;
      uint32_t length = (uint32_t)dec_varint(&buf[i], len - i, &lused);
      i += lused + length;
    } else if (wire_type == 5) {
      i += 4;
    } else {
      break;
    }
  }
}

static void handle_set_config(const uint8_t *cfg_bytes, uint16_t len) {
  uint16_t i = 0;
  while (i < len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&cfg_bytes[i], len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 2) {
      uint16_t lused = 0;
      uint32_t length = (uint32_t)dec_varint(&cfg_bytes[i], len - i, &lused);
      i += lused;
      if (i + length > len)
        break;

      if (field == 1) {
        ESP_LOGI(TAG, "set_config: DeviceConfig (%lu bytes)", (unsigned long)length);
        parse_device_config(&cfg_bytes[i], (uint16_t)length);
      } else if (field == 8) {
        ESP_LOGI(TAG, "set_config: SecurityConfig (%lu bytes)", (unsigned long)length);
        admin_keys_set_from_security_config(&cfg_bytes[i], (uint16_t)length);
      } else {
        ESP_LOGI(TAG,
                 "set_config: section %lu (%lu bytes) - not implemented",
                 (unsigned long)field,
                 (unsigned long)length);
      }
      i += length;
    } else if (wire_type == 0) {
      uint16_t vused = 0;
      dec_varint(&cfg_bytes[i], len - i, &vused);
      i += vused;
    } else if (wire_type == 5) {
      i += 4;
    } else {
      break;
    }
  }
}

static void handle_set_fixed_position(const uint8_t *pos_bytes, uint16_t len) {
  int32_t lat = 0;
  int32_t lon = 0;
  int32_t alt = 0;

  uint16_t i = 0;
  while (i < len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&pos_bytes[i], len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wire_type = (uint32_t)(tag & 0x07);

    if (wire_type == 5) {
      if (i + 4 > len)
        break;
      uint32_t value;
      memcpy(&value, &pos_bytes[i], 4);
      if (field == 1)
        lat = (int32_t)value;
      else if (field == 2)
        lon = (int32_t)value;
      i += 4;
    } else if (wire_type == 0) {
      uint16_t vused = 0;
      uint64_t value = dec_varint(&pos_bytes[i], len - i, &vused);
      i += vused;
      if (field == 3)
        alt = (int32_t)value;
    } else {
      break;
    }
  }
  mt_mod_position_set_fixed(lat, lon, alt);
}

static void dispatch_variant(const mt_packet_meta_t *meta,
                             uint32_t variant_field,
                             uint32_t wire_type,
                             const uint8_t *variant_data,
                             uint16_t variant_len) {
  uint32_t now_s = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
  uint64_t scalar = 0;
  if (wire_type == 0 && variant_data != NULL) {
    uint16_t used = 0;
    scalar = dec_varint(variant_data, variant_len, &used);
  }

  switch (variant_field) {
    case MT_ADMIN_VARIANT_GET_CHANNEL_REQ: {
      if (scalar == 0 || scalar > MT_MAX_CHANNELS) {
        ESP_LOGW(TAG,
                 "get_channel_request=%llu out of 1..%d",
                 (unsigned long long)scalar,
                 MT_MAX_CHANNELS);
        break;
      }
      handle_get_channel(meta, (uint8_t)(scalar - 1), now_s);
      break;
    }
    case MT_ADMIN_VARIANT_GET_OWNER_REQ:
      handle_get_owner(meta, now_s);
      break;
    case MT_ADMIN_VARIANT_GET_CONFIG_REQ: {
      ESP_LOGI(TAG, "get_config_request type=%lu", (unsigned long)scalar);
      uint8_t config[64];
      uint16_t cfg_len = build_config_for_type(config, (uint8_t)scalar);

      uint8_t admin[128];
      uint16_t alen =
          wrap_admin(admin, MT_ADMIN_VARIANT_GET_CONFIG_RESP, config, cfg_len, true, now_s);
      meshtastic_mesh_send_data(meta->from,
                                meta->channel,
                                MT_ADMIN_HOP_LIMIT,
                                MT_PORT_ADMIN,
                                admin,
                                alen,
                                meta->id,
                                false,
                                false);
      break;
    }
    case MT_ADMIN_VARIANT_GET_DEVICE_METADATA_REQ:
      handle_get_device_metadata(meta, now_s);
      break;

    case MT_ADMIN_VARIANT_SET_OWNER:
      handle_set_owner(meta, variant_data, variant_len);
      break;
    case MT_ADMIN_VARIANT_SET_CHANNEL:
      handle_set_channel(variant_data, variant_len);
      break;
    case MT_ADMIN_VARIANT_SET_CONFIG:
      handle_set_config(variant_data, variant_len);
      break;

    case MT_ADMIN_VARIANT_SET_FIXED_POSITION:
      handle_set_fixed_position(variant_data, variant_len);
      break;
    case MT_ADMIN_VARIANT_REMOVE_FIXED_POS:
      mt_mod_position_remove_fixed();
      break;

    case MT_ADMIN_VARIANT_SET_FAVORITE: {
      bool ok = mt_nodedb_set_favorite((uint32_t)scalar, true);
      ESP_LOGI(
          TAG, "set_favorite 0x%08lX -> %s", (unsigned long)scalar, ok ? "ok" : "no such node");
      break;
    }
    case MT_ADMIN_VARIANT_REMOVE_FAVORITE: {
      bool ok = mt_nodedb_set_favorite((uint32_t)scalar, false);
      ESP_LOGI(
          TAG, "remove_favorite 0x%08lX -> %s", (unsigned long)scalar, ok ? "ok" : "no such node");
      break;
    }

    case MT_ADMIN_VARIANT_SET_TIME_ONLY: {
      uint32_t unix_time = 0;
      if (wire_type == 5 && variant_data != NULL && variant_len >= 4) {
        unix_time = (uint32_t)variant_data[0] | ((uint32_t)variant_data[1] << 8) |
                    ((uint32_t)variant_data[2] << 16) | ((uint32_t)variant_data[3] << 24);
      }
      ESP_LOGI(TAG, "set_time_only=%lu (no RTC, ignoring)", (unsigned long)unix_time);
      break;
    }

    case MT_ADMIN_VARIANT_BEGIN_EDIT:
      s_is_edit_transaction_open = true;
      ESP_LOGI(TAG, "begin_edit_settings");
      break;
    case MT_ADMIN_VARIANT_COMMIT_EDIT:
      s_is_edit_transaction_open = false;
      mt_nvs_commit();
      ESP_LOGI(TAG, "commit_edit_settings");
      break;

    case MT_ADMIN_VARIANT_KEY_VERIFICATION: {
      uint8_t message_type = 0;
      uint32_t remote_nodenum = 0;
      uint64_t nonce = 0;
      uint32_t security_number = 0;
      bool has_security = false;

      uint16_t j = 0;
      while (j < variant_len) {
        uint16_t uu = 0;
        uint64_t t = dec_varint(&variant_data[j], variant_len - j, &uu);
        if (uu == 0)
          break;
        j += uu;
        uint32_t f = (uint32_t)(t >> 3);
        uint32_t wt = (uint32_t)(t & 0x07);
        if (wt == 0) {
          uint16_t vu = 0;
          uint64_t v = dec_varint(&variant_data[j], variant_len - j, &vu);
          j += vu;
          if (f == 1)
            message_type = (uint8_t)v;
          else if (f == 2)
            remote_nodenum = (uint32_t)v;
          else if (f == 3)
            nonce = v;
          else if (f == 4) {
            security_number = (uint32_t)v;
            has_security = true;
          }
        } else {
          break;
        }
      }
      mt_mod_keyverify_handle_admin(
          remote_nodenum, message_type, nonce, has_security, security_number);
      break;
    }

    default:
      ESP_LOGI(TAG, "Variant %lu not implemented", (unsigned long)variant_field);
      break;
  }
}

void mt_mod_admin_init(uint32_t node_num) {
  s_node_num = node_num;
  memset(s_session_passkey, 0, sizeof(s_session_passkey));
  s_passkey_set_s = 0;
  s_is_edit_transaction_open = false;
  admin_keys_load();
  ESP_LOGI(TAG, "Initialized (admin_keys: %s)", admin_keys_any_set() ? "configured" : "open mode");
}

void mt_mod_admin_on_received(const mt_packet_meta_t *meta, const uint8_t *payload, uint16_t len) {
  if (meta->to != s_node_num && meta->to != 0xFFFFFFFF) {
    return;
  }

  uint32_t variant_field = 0;
  uint32_t wire_type = 0;
  const uint8_t *variant_data = NULL;
  uint16_t variant_len = 0;
  const uint8_t *passkey = NULL;
  uint16_t passkey_len = 0;
  uint8_t scalar_buf[16];

  uint16_t i = 0;
  while (i < len) {
    uint16_t used = 0;
    uint64_t tag = dec_varint(&payload[i], len - i, &used);
    if (used == 0)
      break;
    i += used;
    uint32_t field = (uint32_t)(tag >> 3);
    uint32_t wt = (uint32_t)(tag & 0x07);

    if (wt == 2) {
      uint16_t len_used = 0;
      uint32_t length = (uint32_t)dec_varint(&payload[i], len - i, &len_used);
      i += len_used;
      if (i + length > len)
        break;
      if (field == MT_ADMIN_VARIANT_SESSION_PASSKEY) {
        passkey = &payload[i];
        passkey_len = (uint16_t)length;
      } else {
        variant_field = field;
        wire_type = 2;
        variant_data = &payload[i];
        variant_len = (uint16_t)length;
      }
      i += length;
    } else if (wt == 0) {
      uint16_t vused = 0;
      uint64_t scalar_val = dec_varint(&payload[i], len - i, &vused);
      i += vused;
      variant_field = field;
      wire_type = 0;
      uint16_t scalar_len = enc_varint(scalar_buf, scalar_val);
      variant_data = scalar_buf;
      variant_len = scalar_len;
    } else if (wt == 5) {
      if (i + 4 > len)
        break;
      variant_field = field;
      wire_type = 5;
      variant_data = &payload[i];
      variant_len = 4;
      i += 4;
    } else {
      break;
    }
  }

  bool requires_passkey = (variant_field >= MT_ADMIN_FIRST_SET_VARIANT);
  bool is_local = (meta->from == 0 || meta->from == s_node_num);

  if (requires_passkey && !is_local) {
    if (!is_passkey_valid(passkey, passkey_len)) {
      ESP_LOGW(
          TAG, "Variant=%lu REJECTED (passkey invalid or expired)", (unsigned long)variant_field);
      return;
    }
    if (!is_sender_admin_authorized(meta->from)) {
      ESP_LOGW(TAG,
               "Variant=%lu REJECTED (sender 0x%08lX not in admin_keys)",
               (unsigned long)variant_field,
               (unsigned long)meta->from);
      return;
    }
  }

  dispatch_variant(meta, variant_field, wire_type, variant_data, variant_len);
}

void mt_mod_admin_tick(void) {}
