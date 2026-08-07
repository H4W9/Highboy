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

#include "mfkey.h"

#include <stdlib.h>
#include <string.h>

#include "crypto1.h"

#define MK_BIT(x, n)   (((x) >> (n)) & 1U)
#define MK_BEBIT(x, n) MK_BIT((x), (n) ^ 24)
#define MK_LFP_ODD     0x29CE5CU
#define MK_LFP_EVEN    0x870804U

#define MK_HALF_STATE_BITS 24

#define MFKEY_MAX_CANDS 4096

#define MK_SIG_CLOCKS 16

#define MK_PRNG_KS2_ADVANCE 64

static uint32_t mk_filter(uint32_t in) {
  uint32_t o = 0;
  o = 0xf22c0U >> (in & 0xfU) & 16U;
  o |= 0x6c9c0U >> (in >> 4 & 0xfU) & 8U;
  o |= 0x3c8b0U >> (in >> 8 & 0xfU) & 4U;
  o |= 0x1e458U >> (in >> 12 & 0xfU) & 2U;
  o |= 0x0d938U >> (in >> 16 & 0xfU) & 1U;
  return MK_BIT(0xEC57E80AU, o);
}

static uint8_t mk_parity32(uint32_t x) {
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return (uint8_t)(x & 1U);
}

static uint8_t mk_rollback_bit(crypto1_state_t *s, uint32_t in, int fb) {
  uint8_t ret;
  uint32_t out;
  uint32_t t = s->odd;
  s->odd = s->even;
  s->even = t;
  out = s->even & 1U;
  out ^= MK_LFP_EVEN & (s->even >>= 1);
  out ^= MK_LFP_ODD & s->odd;
  out ^= (uint32_t)(!!in);
  out ^= (ret = (uint8_t)mk_filter(s->odd)) & (uint32_t)(!!fb);
  s->even |= mk_parity32(out) << (MK_HALF_STATE_BITS - 1);
  return ret;
}

static uint32_t mk_rollback_word(crypto1_state_t *s, uint32_t in, int fb) {
  uint32_t out = 0;
  for (int8_t i = 31; i >= 0; i--)
    out |= (uint32_t)mk_rollback_bit(s, MK_BEBIT(in, i), fb) << (24 ^ (uint8_t)i);
  return out;
}

static uint64_t mk_get_lfsr(const crypto1_state_t *s) {
  uint64_t k = 0;
  for (int8_t i = MK_HALF_STATE_BITS - 1; i >= 0; i--)
    k = k << 2 | (uint64_t)(MK_BIT(s->even, i) << 1) | MK_BIT(s->odd, i);
  return k;
}

static uint32_t mk_simulate_ks(crypto1_state_t s) {
  uint32_t ks = 0;
  for (uint8_t i = 0; i < 32; i++)
    ks |= (uint32_t)crypto1_bit(&s, 0, 0) << (24 ^ i);
  return ks;
}

static size_t mk_gen_odd_candidates(uint32_t ks2, uint32_t *out, size_t out_max) {
  size_t count = 0;
  for (uint32_t O = 0; O < (1U << MK_HALF_STATE_BITS) && count < out_max; O++) {
    uint32_t sig = 0;
    for (uint8_t k = 0; k < MK_SIG_CLOCKS; k++)
      sig |= mk_filter((O << k) & 0xFFFFFFU) << k;
    uint32_t expected = 0;
    for (uint8_t k = 0; k < MK_SIG_CLOCKS; k++)
      expected |= MK_BEBIT(ks2, k * 2) << k;
    if (sig == expected)
      out[count++] = O;
  }
  return count;
}

static size_t mk_gen_even_candidates(uint32_t ks2, uint32_t *out, size_t out_max) {
  size_t count = 0;
  for (uint32_t E = 0; E < (1U << MK_HALF_STATE_BITS) && count < out_max; E++) {
    uint32_t sig = 0;
    for (uint8_t k = 0; k < MK_SIG_CLOCKS; k++)
      sig |= mk_filter((E << (k + 1)) & 0xFFFFFFU) << k;
    uint32_t expected = 0;
    for (uint8_t k = 0; k < MK_SIG_CLOCKS; k++)
      expected |= MK_BEBIT(ks2, k * 2 + 1) << k;
    if (sig == expected)
      out[count++] = E;
  }
  return count;
}

bool mfkey32(uint32_t uid,
             uint32_t nt0,
             uint32_t nr0,
             uint32_t ar0,
             uint32_t nt1,
             uint32_t nr1,
             uint32_t ar1,
             uint64_t *key) {
  if (key == NULL)
    return false;

  uint32_t ks2_0 = ar0 ^ crypto1_prng_successor(nt0, MK_PRNG_KS2_ADVANCE);
  uint32_t ks2_1 = ar1 ^ crypto1_prng_successor(nt1, MK_PRNG_KS2_ADVANCE);

  uint32_t *odd_cands = malloc(sizeof(uint32_t) * MFKEY_MAX_CANDS);
  uint32_t *even_cands = malloc(sizeof(uint32_t) * MFKEY_MAX_CANDS);
  if (odd_cands == NULL || even_cands == NULL) {
    free(odd_cands);
    free(even_cands);
    return false;
  }

  size_t n_odd = mk_gen_odd_candidates(ks2_0, odd_cands, MFKEY_MAX_CANDS);
  size_t n_even = mk_gen_even_candidates(ks2_0, even_cands, MFKEY_MAX_CANDS);

  bool found = false;

  for (size_t oi = 0; oi < n_odd && !found; oi++) {
    for (size_t ei = 0; ei < n_even && !found; ei++) {
      crypto1_state_t s = {odd_cands[oi], even_cands[ei]};

      if (mk_simulate_ks(s) != ks2_0)
        continue;

      mk_rollback_word(&s, 0, 0);
      mk_rollback_word(&s, nr0, 1);
      mk_rollback_word(&s, uid ^ nt0, 0);
      uint64_t key_candidate = mk_get_lfsr(&s);

      crypto1_state_t s1;
      crypto1_init(&s1, key_candidate);
      crypto1_word(&s1, uid ^ nt1, 0);
      crypto1_word(&s1, nr1, 1);
      if (crypto1_word(&s1, 0, 0) == ks2_1) {
        *key = key_candidate;
        found = true;
      }
    }
  }

  free(odd_cands);
  free(even_cands);
  return found;
}
