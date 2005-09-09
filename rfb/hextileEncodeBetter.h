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

#include <map>

#include <rdr/OutStream.h>
#include <rfb/hextileConstants.h>

#include <assert.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define WRITE_PIXEL CONCAT2E(writeOpaque,BPP)
#define HEXTILE_ENCODE CONCAT2E(hextileEncodeBetter,BPP)

/********************************************************************/

#define HEXTILE_SUBRECTS_TABLE CONCAT2E(HextileSubrectsTable,BPP)

class HEXTILE_SUBRECTS_TABLE {

 public:

  HEXTILE_SUBRECTS_TABLE ();

  void newTile(const PIXEL_T *src, int w, int h);
  int buildTables();
  int encode(rdr::U8* dst);

  // Flags can include: hextileAnySubrects, hextileSubrectsColoured
  int getFlags() const { return m_flags; }

  int getBackground() const { return m_background; }
  int getForeground() const { return m_foreground; }

 protected:

  const PIXEL_T *m_tile;
  int m_width;
  int m_height;
  int m_numSubrects;
  int m_flags;
  PIXEL_T m_background;
  PIXEL_T m_foreground;

  // FIXME: Comment data structures.
  rdr::U8 m_coords[256 * 2];
  PIXEL_T m_colors[256];

 private:

  /* DEBUG: Check performance for: (1) U8[] and (2) dyn.allocated. */
  bool m_processed[16][16];

  /* FIXME: Use array for (BPP == 8)? */
  /* DEBUG: Use own hashing like in ZRLE? */
  std::map<PIXEL_T,short> m_counts;
};

HEXTILE_SUBRECTS_TABLE::HEXTILE_SUBRECTS_TABLE()
  : m_tile(NULL), m_width(0), m_height(0), m_numSubrects(0),
    m_flags(0), m_background(0), m_foreground(0)
{
}

void HEXTILE_SUBRECTS_TABLE::newTile(const PIXEL_T *src, int w, int h)
{
  m_tile = src;
  m_width = w;
  m_height = h;
}

/*
 * Returns estimated encoded data size.
 */

int HEXTILE_SUBRECTS_TABLE::buildTables()
{
  assert(m_tile && m_width && m_height);

  m_numSubrects = 0;
  memset(m_processed, 0, 16 * 16 * sizeof(bool));
  m_counts.clear();

  int x, y, sx, sy, sw, sh, max_x;
  PIXEL_T color;
  PIXEL_T *colorsPtr = &m_colors[0];
  rdr::U8 *coordsPtr = &m_coords[0];

  for (y = 0; y < m_height; y++) {
    for (x = 0; x < m_width; x++) {
      /* Skip pixels that were processed earlier */
      if (m_processed[y][x]) {
        continue;
      }
      /* Determine dimensions of the horizontal subrect */
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

      /* Save properties of this subrect */
      *colorsPtr++ = color;
      *coordsPtr++ = (rdr::U8)((x << 4) | (y & 0x0F));
      *coordsPtr++ = (rdr::U8)(((sw - 1) << 4) | ((sh - 1) & 0x0F));
      m_counts[color] += 1;

      m_numSubrects++;

      /* Mark pixels of this subrect as processed, below this row */
      for (sy = y + 1; sy < y + sh; sy++) {
        for (sx = x; sx < x + sw; sx++)
          m_processed[sy][sx] = true;
      }

      /* Skip processed pixels of this row */
      x += (sw - 1);
    }
  }

  // Save the number of colors
  int numColors = m_counts.size();

  // Handle solid tile
  if (numColors == 1) {
    m_background = m_tile[0];
    m_flags = 0;
    return 0;
  }

  std::map<PIXEL_T,short>::iterator i;

  // Handle monochrome tile - choose background and foreground
  if (numColors == 2) {
    i = m_counts.begin();
    m_background = i->first;
    int bgCount = i->second;
    i++;
    if (i->second <= bgCount) {
      m_foreground = i->first;
    } else {
      m_foreground = m_background;
      m_background = i->first;
      bgCount = i->second;;
    }

    m_flags = hextileAnySubrects;
    return 1 + 2 * (m_numSubrects - bgCount);
  }

  // Handle colored tile - choose the best background color
  int bgCount = 0, count;
  for (i = m_counts.begin(); i != m_counts.end(); i++) {
    color = i->first;
    count = i->second;
    if (count > bgCount) {
      bgCount = count;
      m_background = color;
    }
  }

  m_flags = hextileAnySubrects | hextileSubrectsColoured;
  return 1 + (2 + (BPP/8)) * (m_numSubrects - bgCount);
}

/*
 * Call this function only if hextileAnySubrects bit is set in flags.
 * The buffer size should be enough to store at least that number of
 * bytes returned by buildTables() method.
 * Returns encoded data size, or zero if something is wrong.
 */

int HEXTILE_SUBRECTS_TABLE::encode(rdr::U8 *dst)
{
  assert(m_numSubrects && (m_flags & hextileAnySubrects));

  // Zero subrects counter.
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

  return (dst - numSubrectsPtr);
}

/*------------------------------------------------------------------*/

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

  HEXTILE_SUBRECTS_TABLE subrects;

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

    t.br.y = vncmin(r.br.y, t.tl.y + 16);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {

      t.br.x = vncmin(r.br.x, t.tl.x + 16);

      GET_IMAGE_INTO_BUF(t,buf);

      subrects.newTile(buf, t.width(), t.height());
      int encodedLen = subrects.buildTables();

      if (encodedLen >= t.width() * t.height() * (BPP/8)) {
        os->writeU8(hextileRaw);
        os->writeBytes(buf, t.width() * t.height() * (BPP/8));
        oldBgValid = oldFgValid = false;
        continue;
      }

      int tileType = subrects.getFlags();
      PIXEL_T bg = subrects.getBackground();
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
          fg = subrects.getForeground();
          if (!oldFgValid || oldFg != fg) {
            tileType |= hextileFgSpecified;
            oldFg = fg;
            oldFgValid = true;
          }
        }
        int finalEncodedLen = subrects.encode(encoded);
        assert(finalEncodedLen == encodedLen);
        assert(finalEncodedLen <= 256*(BPP/8));
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
#undef HEXTILE_ENCODE

#undef HEXTILE_SUBRECTS_TABLE
}
