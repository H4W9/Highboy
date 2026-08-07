// Copyright (c) 2026 HIGH CODE LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef RNODE_KISS_H
#define RNODE_KISS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define KISS_FEND  0xC0
#define KISS_FESC  0xDB
#define KISS_TFEND 0xDC
#define KISS_TFESC 0xDD

#define RNODE_KISS_FRAME_MAX 512

/**
 * @brief State machine for the KISS frame decoder.
 *
 * The decoder is byte-driven. When a full frame becomes available the
 * caller drains it via rnode_kiss_decoder_take_frame.
 */
typedef struct {
  uint8_t buf[RNODE_KISS_FRAME_MAX];
  size_t len;
  bool is_in_frame;
  bool is_escaped;
  bool has_frame_ready;
} rnode_kiss_decoder_t;

/**
 * @brief Reset the decoder to its initial state.
 *
 * @param d  Decoder instance. Must not be NULL.
 */
void rnode_kiss_decoder_reset(rnode_kiss_decoder_t *d);

/**
 * @brief Feed one received byte to the decoder.
 *
 * @param d     Decoder instance.
 * @param byte  Byte received from serial.
 * @return true if a complete frame became available after this byte.
 */
bool rnode_kiss_decoder_feed(rnode_kiss_decoder_t *d, uint8_t byte);

/**
 * @brief Pop the ready frame and clear the flag.
 *
 * @param[in]  d        Decoder.
 * @param[out] out_buf  Destination buffer (full frame: cmd + payload).
 * @param      cap      Destination capacity.
 * @param[out] out_len  Frame size written to out_buf.
 * @return true if a frame was popped, false otherwise.
 */
bool rnode_kiss_decoder_take_frame(rnode_kiss_decoder_t *d,
                                   uint8_t *out_buf,
                                   size_t cap,
                                   size_t *out_len);

/**
 * @brief Encode a KISS frame ready for TX on the serial link.
 *
 * Wraps payload with FEND markers and escapes special bytes.
 *
 * @param[in]  cmd_id       First logical byte of the frame.
 * @param[in]  payload      Data buffer (may be NULL if payload_len == 0).
 * @param      payload_len  Payload size in bytes.
 * @param[out] out_buf      Destination for the encoded frame.
 * @param      cap          Capacity of out_buf.
 * @return Encoded length, or 0 if cap is exceeded.
 */
size_t rnode_kiss_encode(
    uint8_t cmd_id, const uint8_t *payload, size_t payload_len, uint8_t *out_buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif // RNODE_KISS_H
