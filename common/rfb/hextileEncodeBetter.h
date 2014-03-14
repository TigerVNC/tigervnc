/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Constantin Kaplinsky.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */
//
// Hextile encoding function.
//
// This file is #included after having set the following macro:
// BPP                - 8, 16 or 32

#include <rdr/OutStream.h>
#include <rfb/hextileConstants.h>
#include <rfb/Palette.h>

#include <assert.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define WRITE_PIXEL CONCAT2E(writeOpaque,BPP)
#define HEXTILE_TILE CONCAT2E(HextileTile,BPP)
#define HEXTILE_ENCODE CONCAT2E(hextileEncodeBetter,BPP)

//
// This class analyzes a separate tile and encodes its subrectangles.
//

class HEXTILE_TILE {

 public:

  HEXTILE_TILE ();

  //
  // Initialize existing object instance with new tile data.
  //
  void newTile(const PIXEL_T *src, int w, int h);

  //
  // Flags can include: hextileRaw, hextileAnySubrects and
  // hextileSubrectsColoured. Note that if hextileRaw is set, other
  // flags make no sense. Also, hextileSubrectsColoured is meaningful
  // only when hextileAnySubrects is set as well.
  //
  int getFlags() const { return m_flags; }

  //
  // Returns the size of encoded subrects data, including subrect count.
  // The size is zero if flags do not include hextileAnySubrects.
  //
  int getSize() const { return m_size; }

  //
  // Return optimal background.
  //
  int getBackground() const { return m_background; }

  //
  // Return foreground if flags include hextileSubrectsColoured.
  //
  int getForeground() const { return m_foreground; }

  //
  // Encode subrects. This function may be called only if
  // hextileAnySubrects bit is set in flags. The buffer size should be
  // big enough to store at least the number of bytes returned by the
  // getSize() method.
  //
  void encode(rdr::U8* dst) const;

 protected:

  //
  // Analyze the tile pixels, fill in all the data fields.
  //
  void analyze();

  const PIXEL_T *m_tile;
  int m_width;
  int m_height;

  int m_size;
  int m_flags;
  PIXEL_T m_background;
  PIXEL_T m_foreground;

  int m_numSubrects;
  rdr::U8 m_coords[256 * 2];
  PIXEL_T m_colors[256];

 private:

  bool m_processed[16][16];
  Palette m_pal;
};

HEXTILE_TILE::HEXTILE_TILE()
  : m_tile(NULL), m_width(0), m_height(0),
    m_size(0), m_flags(0), m_background(0), m_foreground(0),
    m_numSubrects(0)
{
}

void HEXTILE_TILE::newTile(const PIXEL_T *src, int w, int h)
{
  m_tile = src;
  m_width = w;
  m_height = h;

  analyze();
}

void HEXTILE_TILE::analyze()
{
  assert(m_tile && m_width && m_height);

  const PIXEL_T *ptr = m_tile;
  const PIXEL_T *end = &m_tile[m_width * m_height];
  PIXEL_T color = *ptr++;
  while (ptr != end && *ptr == color)
    ptr++;

  // Handle solid tile
  if (ptr == end) {
    m_background = m_tile[0];
    m_flags = 0;
    m_size = 0;
    return;
  }

  // Compute number of complete rows of the same color, at the top
  int y = (ptr - m_tile) / m_width;

  PIXEL_T *colorsPtr = m_colors;
  rdr::U8 *coordsPtr = m_coords;
  m_pal.clear();
  m_numSubrects = 0;

  // Have we found the first subrect already?
  if (y > 0) {
    *colorsPtr++ = color;
    *coordsPtr++ = 0;
    *coordsPtr++ = (rdr::U8)(((m_width - 1) << 4) | ((y - 1) & 0x0F));
    m_pal.insert(color, 1);
    m_numSubrects++;
  }

  memset(m_processed, 0, 16 * 16 * sizeof(bool));

  int x, sx, sy, sw, sh, max_x;

  for (; y < m_height; y++) {
    for (x = 0; x < m_width; x++) {
      // Skip pixels that were processed earlier
      if (m_processed[y][x]) {
        continue;
      }
      // Determine dimensions of the horizontal subrect
      color = m_tile[y * m_width + x];
      for (sx = x + 1; sx < m_width; sx++) {
        if (m_tile[y * m_width + sx] != color)
          break;
      }
      sw = sx - x;
      max_x = sx;
      for (sy = y + 1; sy < m_height; sy++) {
        for (sx = x; sx < max_x; sx++) {
          if (m_tile[sy * m_width + sx] != color)
            goto done;
        }
      }
    done:
      sh = sy - y;

      // Save properties of this subrect
      *colorsPtr++ = color;
      *coordsPtr++ = (rdr::U8)((x << 4) | (y & 0x0F));
      *coordsPtr++ = (rdr::U8)(((sw - 1) << 4) | ((sh - 1) & 0x0F));

      if (!m_pal.insert(color, 1) || (m_pal.size() > (48 + 2 * BPP))) {
        // Handle palette overflow
        m_flags = hextileRaw;
        m_size = 0;
        return;
      }

      m_numSubrects++;

      // Mark pixels of this subrect as processed, below this row
      for (sy = y + 1; sy < y + sh; sy++) {
        for (sx = x; sx < x + sw; sx++)
          m_processed[sy][sx] = true;
      }

      // Skip processed pixels of this row
      x += (sw - 1);
    }
  }

  // Save number of colors in this tile (should be no less than 2)
  int numColors = m_pal.size();
  assert(numColors >= 2);

  m_background = (PIXEL_T)m_pal.getColour(0);
  m_flags = hextileAnySubrects;
  int numSubrects = m_numSubrects - m_pal.getCount(0);

  if (numColors == 2) {
    // Monochrome tile
    m_foreground = (PIXEL_T)m_pal.getColour(1);
    m_size = 1 + 2 * numSubrects;
  } else {
    // Colored tile
    m_flags |= hextileSubrectsColoured;
    m_size = 1 + (2 + (BPP/8)) * numSubrects;
  }
}

void HEXTILE_TILE::encode(rdr::U8 *dst) const
{
  assert(m_numSubrects && (m_flags & hextileAnySubrects));

  // Zero subrects counter
  rdr::U8 *numSubrectsPtr = dst;
  *dst++ = 0;

  for (int i = 0; i < m_numSubrects; i++) {
    if (m_colors[i] == m_background)
      continue;

    if (m_flags & hextileSubrectsColoured) {
#if (BPP == 8)
      *dst++ = m_colors[i];
#elif (BPP == 16)
      *dst++ = ((rdr::U8*)&m_colors[i])[0];
      *dst++ = ((rdr::U8*)&m_colors[i])[1];
#elif (BPP == 32)
      *dst++ = ((rdr::U8*)&m_colors[i])[0];
      *dst++ = ((rdr::U8*)&m_colors[i])[1];
      *dst++ = ((rdr::U8*)&m_colors[i])[2];
      *dst++ = ((rdr::U8*)&m_colors[i])[3];
#endif
    }
    *dst++ = m_coords[i * 2];
    *dst++ = m_coords[i * 2 + 1];

    (*numSubrectsPtr)++;
  }

  assert(dst - numSubrectsPtr == m_size);
}

//
// Main encoding function.
//

void HEXTILE_ENCODE(rdr::OutStream* os, const PixelBuffer* pb)
{
  Rect t;
  PIXEL_T buf[256];
  PIXEL_T oldBg = 0, oldFg = 0;
  bool oldBgValid = false;
  bool oldFgValid = false;
  rdr::U8 encoded[256*(BPP/8)];

  HEXTILE_TILE tile;

  for (t.tl.y = 0; t.tl.y < pb->height(); t.tl.y += 16) {

    t.br.y = __rfbmin(pb->height(), t.tl.y + 16);

    for (t.tl.x = 0; t.tl.x < pb->width(); t.tl.x += 16) {

      t.br.x = __rfbmin(pb->width(), t.tl.x + 16);

      pb->getImage(buf, t);

      tile.newTile(buf, t.width(), t.height());
      int tileType = tile.getFlags();
      int encodedLen = tile.getSize();

      if ( (tileType & hextileRaw) != 0 ||
           encodedLen >= t.width() * t.height() * (BPP/8)) {
        os->writeU8(hextileRaw);
        os->writeBytes(buf, t.width() * t.height() * (BPP/8));
        oldBgValid = oldFgValid = false;
        continue;
      }

      PIXEL_T bg = tile.getBackground();
      PIXEL_T fg = 0;

      if (!oldBgValid || oldBg != bg) {
        tileType |= hextileBgSpecified;
        oldBg = bg;
        oldBgValid = true;
      }

      if (tileType & hextileAnySubrects) {
        if (tileType & hextileSubrectsColoured) {
          oldFgValid = false;
        } else {
          fg = tile.getForeground();
          if (!oldFgValid || oldFg != fg) {
            tileType |= hextileFgSpecified;
            oldFg = fg;
            oldFgValid = true;
          }
        }
        tile.encode(encoded);
      }

      os->writeU8(tileType);
      if (tileType & hextileBgSpecified) os->WRITE_PIXEL(bg);
      if (tileType & hextileFgSpecified) os->WRITE_PIXEL(fg);
      if (tileType & hextileAnySubrects) os->writeBytes(encoded, encodedLen);
    }
  }
}

#undef PIXEL_T
#undef WRITE_PIXEL
#undef HEXTILE_TILE
#undef HEXTILE_ENCODE
}
