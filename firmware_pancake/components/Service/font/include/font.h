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

#ifndef FONT_H
#define FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Bitmap font descriptor for fixed-width rendering.
 */
typedef struct {
  const uint8_t *bitmap; /**< Contiguous glyph bitmap data.       */
  uint16_t first_char;   /**< First character code (ASCII).        */
  uint16_t last_char;    /**< Last character code (ASCII).         */
  uint8_t width;         /**< Glyph width in pixels.               */
  uint8_t height;        /**< Glyph height in pixels.              */
  uint8_t spacing;       /**< Horizontal spacing between glyphs.   */
} font_t;

/** @brief Ubuntu Mono 16px bitmap font (ASCII 32-126). */
extern const font_t font_ubuntu_16;

/** @brief Terminus 24px bitmap font. */
extern const font_t font_terminus_24;

/** @brief Arial 32px bitmap font. */
extern const font_t font_arial_32;

#ifdef __cplusplus
}
#endif

#endif /* FONT_H */
