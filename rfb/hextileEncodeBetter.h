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
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// GET_IMAGE_INTO_BUF - gets a rectangle of pixel data into a buffer

#include <rdr/OutStream.h>
#include <rfb/hextileConstants.h>
#include <rfb/TightPalette.h>

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
  // Fill in m_coords[], m_colors[], m_pal and set m_numSubrects.
  //
  void buildTables();

  //
  // Fill in m_size, m_flags, m_background and m_foreground.
  // Must be called after buildTables(), before encode().
  //
  void computeSize();

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
  TightPalette m_pal;
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

  // NOTE: These two methods should always be called from this place
  //       only, and exactly in this order.
  buildTables();
  computeSize();
}

void HEXTILE_TILE::buildTables()
{
  assert(m_tile && m_width && m_height);

  m_numSubrects = 0;
  memset(m_processed, 0, 16 * 16 * sizeof(bool));
  m_pal.reset();

  int x, y, sx, sy, sw, sh, max_x;
  PIXEL_T color;
  PIXEL_T *colorsPtr = &m_colors[0];
  rdr::U8 *coordsPtr = &m_coords[0];

  for (y = 0; y < m_height; y++) {
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

      if (m_pal.insert(color, 1, BPP) == 0)
        return;                 // palette overflow

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
}

void HEXTILE_TILE::computeSize()
{
  // Save the number of colors
  int numColors = m_pal.getNumColors();

  // Handle solid tile
  if (numColors == 1) {
    m_background = m_tile[0];
    m_flags = 0;
    m_size = 0;
    return;
  }

  // Handle monochrome tile - choose background and foreground
  if (numColors == 2) {
    int bgCount = m_pal.getCount(0);
    m_background = (PIXEL_T)m_pal.getEntry(0);
    m_foreground = (PIXEL_T)m_pal.getEntry(1);

    m_flags = hextileAnySubrects;
    m_size = 1 + 2 * (m_numSubrects - bgCount);
    return;
  }

  // Handle raw-encoded tile (there was palette overflow)
  if (numColors == 0) {
    m_flags = hextileRaw;
    m_size = 0;
    return;
  }

  // Handle colored tile - choose the best background color
  int bgCount = m_pal.getCount(0);
  m_background = (PIXEL_T)m_pal.getEntry(0);

  m_flags = hextileAnySubrects | hextileSubrectsColoured;
  m_size = 1 + (2 + (BPP/8)) * (m_numSubrects - bgCount);
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

void HEXTILE_ENCODE(const Rect& r, rdr::OutStream* os
#ifdef EXTRA_ARGS
                    , EXTRA_ARGS
#endif
                    )
{
  Rect t;
  PIXEL_T buf[256];
  PIXEL_T oldBg = 0, oldFg = 0;
  bool oldBgValid = false;
  bool oldFgValid = false;
  rdr::U8 encoded[256*(BPP/8)];

  HEXTILE_TILE tile;

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

    t.br.y = vncmin(r.br.y, t.tl.y + 16);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {

      t.br.x = vncmin(r.br.x, t.tl.x + 16);

      GET_IMAGE_INTO_BUF(t,buf);

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
