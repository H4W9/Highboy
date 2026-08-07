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

#include "meshtastic_crypto_pki.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/ccm.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/bignum.h"
#include "mbedtls/sha256.h"

#include "meshtastic_pki.h"

static const char *TAG = "MT_CRYPTO_PKI";

#define MT_PKI_AES_KEY_BITS 256
#define MT_PKI_NONCE_LEN    13
#define MT_PKI_TAG_LEN      8
#define MT_PKI_EXTRA_LEN    4

/* X25519 manual implementation following RFC 7748.
 * Uses mbedtls_mpi for big-number arithmetic but bypasses the broken
 * mbedtls_ecp_mul path on ESP-IDF Curve25519. */
static int x25519_scalarmult(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32]) {
  int rc = 0;
  mbedtls_mpi p, a24, x_1, x_2, z_2, x_3, z_3;
  mbedtls_mpi A, AA, B, BB, E, C, D, DA, CB, t1, t2;

  mbedtls_mpi *all[] = {
      &p, &a24, &x_1, &x_2, &z_2, &x_3, &z_3, &A, &AA, &B, &BB, &E, &C, &D, &DA, &CB, &t1, &t2};
  for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
    mbedtls_mpi_init(all[i]);
  }

  /* p = 2^255 - 19 */
  if (rc == 0)
    rc = mbedtls_mpi_lset(&p, 1);
  if (rc == 0)
    rc = mbedtls_mpi_shift_l(&p, 255);
  if (rc == 0)
    rc = mbedtls_mpi_sub_int(&p, &p, 19);
  if (rc == 0)
    rc = mbedtls_mpi_lset(&a24, 121665);

  /* Mask high bit of u-coordinate per RFC 7748 */
  uint8_t u[32];
  memcpy(u, point, 32);
  u[31] &= 0x7F;
  if (rc == 0)
    rc = mbedtls_mpi_read_binary_le(&x_1, u, 32);

  /* Initial state */
  if (rc == 0)
    rc = mbedtls_mpi_lset(&x_2, 1);
  if (rc == 0)
    rc = mbedtls_mpi_lset(&z_2, 0);
  if (rc == 0)
    rc = mbedtls_mpi_copy(&x_3, &x_1);
  if (rc == 0)
    rc = mbedtls_mpi_lset(&z_3, 1);

  int swap = 0;

  for (int t = 254; t >= 0 && rc == 0; t--) {
    int k_t = (scalar[t / 8] >> (t & 7)) & 1;
    int sw = swap ^ k_t;
    if (sw) {
      mbedtls_mpi_swap(&x_2, &x_3);
      mbedtls_mpi_swap(&z_2, &z_3);
    }
    swap = k_t;

    /* A = x_2 + z_2 */
    if (rc == 0)
      rc = mbedtls_mpi_add_mpi(&A, &x_2, &z_2);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&A, &A, &p);
    /* AA = A^2 */
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&AA, &A, &A);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&AA, &AA, &p);
    /* B = x_2 - z_2 */
    if (rc == 0)
      rc = mbedtls_mpi_sub_mpi(&B, &x_2, &z_2);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&B, &B, &p);
    /* BB = B^2 */
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&BB, &B, &B);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&BB, &BB, &p);
    /* E = AA - BB */
    if (rc == 0)
      rc = mbedtls_mpi_sub_mpi(&E, &AA, &BB);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&E, &E, &p);
    /* C = x_3 + z_3 */
    if (rc == 0)
      rc = mbedtls_mpi_add_mpi(&C, &x_3, &z_3);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&C, &C, &p);
    /* D = x_3 - z_3 */
    if (rc == 0)
      rc = mbedtls_mpi_sub_mpi(&D, &x_3, &z_3);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&D, &D, &p);
    /* DA = D * A */
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&DA, &D, &A);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&DA, &DA, &p);
    /* CB = C * B */
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&CB, &C, &B);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&CB, &CB, &p);
    /* x_3 = (DA + CB)^2 */
    if (rc == 0)
      rc = mbedtls_mpi_add_mpi(&t1, &DA, &CB);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&t1, &t1, &p);
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&x_3, &t1, &t1);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&x_3, &x_3, &p);
    /* z_3 = x_1 * (DA - CB)^2 */
    if (rc == 0)
      rc = mbedtls_mpi_sub_mpi(&t1, &DA, &CB);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&t1, &t1, &p);
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&t2, &t1, &t1);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&t2, &t2, &p);
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&z_3, &t2, &x_1);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&z_3, &z_3, &p);
    /* x_2 = AA * BB */
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&x_2, &AA, &BB);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&x_2, &x_2, &p);
    /* z_2 = E * (AA + a24 * E) */
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&t1, &a24, &E);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&t1, &t1, &p);
    if (rc == 0)
      rc = mbedtls_mpi_add_mpi(&t1, &t1, &AA);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&t1, &t1, &p);
    if (rc == 0)
      rc = mbedtls_mpi_mul_mpi(&z_2, &t1, &E);
    if (rc == 0)
      rc = mbedtls_mpi_mod_mpi(&z_2, &z_2, &p);
  }

  if (swap && rc == 0) {
    mbedtls_mpi_swap(&x_2, &x_3);
    mbedtls_mpi_swap(&z_2, &z_3);
  }

  /* result = x_2 * z_2^(p - 2) mod p (Fermat inverse) */
  if (rc == 0)
    rc = mbedtls_mpi_sub_int(&t1, &p, 2);
  if (rc == 0)
    rc = mbedtls_mpi_exp_mod(&t2, &z_2, &t1, &p, NULL);
  if (rc == 0)
    rc = mbedtls_mpi_mul_mpi(&t1, &x_2, &t2);
  if (rc == 0)
    rc = mbedtls_mpi_mod_mpi(&t1, &t1, &p);
  if (rc == 0)
    rc = mbedtls_mpi_write_binary_le(&t1, out, 32);

  for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
    mbedtls_mpi_free(all[i]);
  }
  return rc;
}

static bool compute_shared_key(const uint8_t *peer_pub, uint8_t shared_key[32]) {
  const uint8_t *priv = mt_pki_get_privkey();

  uint8_t scalar[32];
  memcpy(scalar, priv, 32);
  scalar[0] &= 0xF8;
  scalar[31] &= 0x7F;
  scalar[31] |= 0x40;

  uint8_t raw_secret[32];
  int rc = x25519_scalarmult(raw_secret, scalar, peer_pub);
  if (rc != 0) {
    ESP_LOGW(TAG, "x25519_scalarmult failed rc=%d (-0x%04X)", rc, -rc);
    return false;
  }

  rc = mbedtls_sha256(raw_secret, 32, shared_key, 0);
  if (rc != 0) {
    ESP_LOGW(TAG, "shared_key sha256 failed rc=%d", rc);
    return false;
  }
  ESP_LOGI(TAG,
           "ECDH OK (X25519 manual) shared[0..3]=%02x%02x%02x%02x",
           shared_key[0],
           shared_key[1],
           shared_key[2],
           shared_key[3]);
  return true;
}

static void build_nonce(uint8_t nonce[MT_PKI_NONCE_LEN],
                        uint32_t from_node,
                        uint32_t packet_id,
                        uint32_t extra_nonce) {
  memset(nonce, 0, MT_PKI_NONCE_LEN);

  uint64_t packet_id_64 = (uint64_t)packet_id;
  memcpy(nonce, &packet_id_64, sizeof(uint64_t));

  memcpy(nonce + 8, &from_node, sizeof(uint32_t));

  if (extra_nonce != 0) {
    memcpy(nonce + 4, &extra_nonce, sizeof(uint32_t));
  }
}

bool mt_pki_encrypt_curve25519(const uint8_t *peer_pub,
                               uint32_t from_node,
                               uint32_t packet_id,
                               const uint8_t *plain,
                               size_t plen,
                               uint8_t *out) {
  uint8_t shared_key[32];
  if (!compute_shared_key(peer_pub, shared_key))
    return false;

  uint32_t extra_nonce = esp_random();
  if (extra_nonce == 0)
    extra_nonce = 1;

  uint8_t nonce[MT_PKI_NONCE_LEN];
  build_nonce(nonce, from_node, packet_id, extra_nonce);

  mbedtls_ccm_context ccm;
  mbedtls_ccm_init(&ccm);
  int rc = mbedtls_ccm_setkey(&ccm, MBEDTLS_CIPHER_ID_AES, shared_key, MT_PKI_AES_KEY_BITS);
  if (rc == 0) {
    rc = mbedtls_ccm_encrypt_and_tag(
        &ccm, plen, nonce, MT_PKI_NONCE_LEN, NULL, 0, plain, out, out + plen, MT_PKI_TAG_LEN);
  }
  mbedtls_ccm_free(&ccm);

  if (rc != 0) {
    ESP_LOGW(TAG, "CCM encrypt failed rc=%d", rc);
    return false;
  }

  memcpy(out + plen + MT_PKI_TAG_LEN, &extra_nonce, MT_PKI_EXTRA_LEN);
  return true;
}

bool mt_pki_decrypt_curve25519(const uint8_t *peer_pub,
                               uint32_t from_node,
                               uint32_t packet_id,
                               const uint8_t *cipher,
                               size_t clen,
                               uint8_t *out) {
  if (clen <= MT_PKI_OVERHEAD)
    return false;

  size_t plen = clen - MT_PKI_OVERHEAD;
  const uint8_t *tag = cipher + plen;
  uint32_t extra_nonce;
  memcpy(&extra_nonce, cipher + plen + MT_PKI_TAG_LEN, MT_PKI_EXTRA_LEN);

  uint8_t shared_key[32];
  if (!compute_shared_key(peer_pub, shared_key))
    return false;

  uint8_t nonce[MT_PKI_NONCE_LEN];
  build_nonce(nonce, from_node, packet_id, extra_nonce);

  mbedtls_ccm_context ccm;
  mbedtls_ccm_init(&ccm);
  int rc = mbedtls_ccm_setkey(&ccm, MBEDTLS_CIPHER_ID_AES, shared_key, MT_PKI_AES_KEY_BITS);
  if (rc == 0) {
    rc = mbedtls_ccm_auth_decrypt(
        &ccm, plen, nonce, MT_PKI_NONCE_LEN, NULL, 0, cipher, out, tag, MT_PKI_TAG_LEN);
  }
  mbedtls_ccm_free(&ccm);

  if (rc != 0) {
    ESP_LOGD(TAG, "CCM decrypt failed rc=%d (probably not PKI)", rc);
    return false;
  }
  return true;
}
