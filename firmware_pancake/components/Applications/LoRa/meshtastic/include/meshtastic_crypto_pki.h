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

#ifndef MESHTASTIC_CRYPTO_PKI_H
#define MESHTASTIC_CRYPTO_PKI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MT_PKI_OVERHEAD 12 /* 8-byte AES-CCM tag + 4-byte extra nonce */

/**
 * @brief Encrypt a payload using X25519 + AES-256-CCM (Meshtastic PKI).
 *
 * Derives shared_key = SHA256(X25519(our_priv, peer_pub)), uses AES-256-CCM
 * with 13-byte nonce built from (packet_id, from_node, random extra_nonce).
 * Output layout: [ciphertext(plen)] [auth_tag(8)] [extra_nonce(4)].
 *
 * @param peer_pub   32-byte Curve25519 public key of destination
 * @param from_node  Sender node number (included in nonce)
 * @param packet_id  Packet id (included in nonce)
 * @param plain      Plaintext input
 * @param plen       Plaintext length
 * @param out        Output buffer (must be at least plen + MT_PKI_OVERHEAD)
 * @return true on success, false on failure (weak key, mbedtls error)
 */
bool mt_pki_encrypt_curve25519(const uint8_t *peer_pub,
                               uint32_t from_node,
                               uint32_t packet_id,
                               const uint8_t *plain,
                               size_t plen,
                               uint8_t *out);

/**
 * @brief Decrypt a payload produced by mt_pki_encrypt_curve25519.
 *
 * Reads extra_nonce from cipher[clen-4..clen-1] and auth_tag from
 * cipher[clen-12..clen-5], derives shared_key, and AEAD-decrypts.
 *
 * @param peer_pub   32-byte Curve25519 public key of sender
 * @param from_node  Sender node number
 * @param packet_id  Packet id
 * @param cipher     Ciphertext input (including trailing tag + extra_nonce)
 * @param clen       Total length including MT_PKI_OVERHEAD
 * @param out        Output buffer (must be at least clen - MT_PKI_OVERHEAD)
 * @return true on success (auth tag valid), false on failure
 */
bool mt_pki_decrypt_curve25519(const uint8_t *peer_pub,
                               uint32_t from_node,
                               uint32_t packet_id,
                               const uint8_t *cipher,
                               size_t clen,
                               uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif // MESHTASTIC_CRYPTO_PKI_H
