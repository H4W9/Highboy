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

#include "meshtastic_pki.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/ecp.h"
#include "mbedtls/bignum.h"

#include "meshtastic_nvs.h"

static const char *TAG = "MT_PKI";

#define MT_PKI_NVS_PRIV "x25519_priv"
#define MT_PKI_NVS_PUB  "x25519_pub"

static uint8_t s_priv[MT_PKI_KEY_LEN];
static uint8_t s_pub[MT_PKI_KEY_LEN];

static int rng_cb(void *ctx, unsigned char *out, size_t len) {
  (void)ctx;
  esp_fill_random(out, len);
  return 0;
}

static esp_err_t generate_keypair(uint8_t *priv_out, uint8_t *pub_out) {
  mbedtls_ecp_group grp;
  mbedtls_mpi d;
  mbedtls_ecp_point Q;

  mbedtls_ecp_group_init(&grp);
  mbedtls_mpi_init(&d);
  mbedtls_ecp_point_init(&Q);

  int rc = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
  if (rc == 0)
    rc = mbedtls_ecp_gen_keypair(&grp, &d, &Q, rng_cb, NULL);
  if (rc == 0)
    rc = mbedtls_mpi_write_binary_le(&d, priv_out, MT_PKI_KEY_LEN);
  if (rc == 0)
    rc = mbedtls_mpi_write_binary_le(&Q.MBEDTLS_PRIVATE(X), pub_out, MT_PKI_KEY_LEN);

  mbedtls_ecp_point_free(&Q);
  mbedtls_mpi_free(&d);
  mbedtls_ecp_group_free(&grp);

  return (rc == 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t mt_pki_init(void) {
  int priv_len = mt_nvs_get_blob(MT_PKI_NVS_PRIV, s_priv, sizeof(s_priv));
  int pub_len = mt_nvs_get_blob(MT_PKI_NVS_PUB, s_pub, sizeof(s_pub));

  if (priv_len == MT_PKI_KEY_LEN && pub_len == MT_PKI_KEY_LEN) {
    ESP_LOGI(TAG,
             "Loaded X25519 keypair from NVS (pub[0..3]=%02x%02x%02x%02x)",
             s_pub[0],
             s_pub[1],
             s_pub[2],
             s_pub[3]);
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Generating new X25519 keypair");
  esp_err_t ret = generate_keypair(s_priv, s_pub);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Keygen failed");
    memset(s_priv, 0, sizeof(s_priv));
    memset(s_pub, 0, sizeof(s_pub));
    return ret;
  }

  mt_nvs_set_blob(MT_PKI_NVS_PRIV, s_priv, sizeof(s_priv));
  mt_nvs_set_blob(MT_PKI_NVS_PUB, s_pub, sizeof(s_pub));

  ESP_LOGI(TAG,
           "Keypair generated and persisted (pub[0..3]=%02x%02x%02x%02x)",
           s_pub[0],
           s_pub[1],
           s_pub[2],
           s_pub[3]);
  return ESP_OK;
}

const uint8_t *mt_pki_get_pubkey(void) {
  return s_pub;
}
const uint8_t *mt_pki_get_privkey(void) {
  return s_priv;
}
