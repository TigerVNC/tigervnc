/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2014-2022 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/HextileEncoder.h>
#include <rfb/Palette.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Configuration.h>
#include <rfb/hextileConstants.h>

using namespace rfb;

BoolParameter improvedHextile("ImprovedHextile",
                              "Use improved compression algorithm for Hextile "
                              "encoding which achieves better compression "
                              "ratios by the cost of using more CPU time",
                              true);

HextileEncoder::HextileEncoder(SConnection* conn_) :
  Encoder(conn_, encodingHextile, EncoderPlain)
{
}

HextileEncoder::~HextileEncoder()
{
}

bool HextileEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingHextile);
}

void HextileEncoder::writeRect(const PixelBuffer* pb,
                               const Palette& /*palette*/)
{
  rdr::OutStream* os = conn->getOutStream();
  switch (pb->getPF().bpp) {
  case 8:
    if (improvedHextile) {
      hextileEncodeBetter<uint8_t>(os, pb);
    } else {
      hextileEncode<uint8_t>(os, pb);
    }
    break;
  case 16:
    if (improvedHextile) {
      hextileEncodeBetter<uint16_t>(os, pb);
    } else {
      hextileEncode<uint16_t>(os, pb);
    }
    break;
  case 32:
    if (improvedHextile) {
      hextileEncodeBetter<uint32_t>(os, pb);
    } else {
      hextileEncode<uint32_t>(os, pb);
    }
    break;
  }
}

void HextileEncoder::writeSolidRect(int width, int height,
                                    const PixelFormat& pf,
                                    const uint8_t* colour)
{
  rdr::OutStream* os;
  int tiles;

  os = conn->getOutStream();

  tiles = ((width + 15)/16) * ((height + 15)/16);

  os->writeU8(hextileBgSpecified);
  os->writeBytes(colour, pf.bpp/8);
  tiles--;

  while (tiles--)
      os->writeU8(0);
}

template<class T>
inline void HextileEncoder::writePixel(rdr::OutStream* os, T pixel)
{
  if (sizeof(T) == 1)
    os->writeOpaque8(pixel);
  else if (sizeof(T) == 2)
    os->writeOpaque16(pixel);
  else if (sizeof(T) == 4)
    os->writeOpaque32(pixel);
}

template<class T>
void HextileEncoder::hextileEncode(rdr::OutStream* os,
                                   const PixelBuffer* pb)
{
  Rect t;
  T buf[256];
  T oldBg = 0, oldFg = 0;
  bool oldBgValid = false;
  bool oldFgValid = false;
  uint8_t encoded[256*sizeof(T)];

  for (t.tl.y = 0; t.tl.y < pb->height(); t.tl.y += 16) {

    t.br.y = __rfbmin(pb->height(), t.tl.y + 16);

    for (t.tl.x = 0; t.tl.x < pb->width(); t.tl.x += 16) {

      t.br.x = __rfbmin(pb->width(), t.tl.x + 16);

      pb->getImage(buf, t);

      T bg = 0, fg = 0;
      int tileType = testTileType(buf, t.width(), t.height(), &bg, &fg);

      if (!oldBgValid || oldBg != bg) {
        tileType |= hextileBgSpecified;
        oldBg = bg;
        oldBgValid = true;
      }

      int encodedLen = 0;

      if (tileType & hextileAnySubrects) {

        if (tileType & hextileSubrectsColoured) {
          oldFgValid = false;
        } else {
          if (!oldFgValid || oldFg != fg) {
            tileType |= hextileFgSpecified;
            oldFg = fg;
            oldFgValid = true;
          }
        }

        encodedLen = hextileEncodeTile(buf, t.width(), t.height(),
                                       tileType, encoded, bg);

        if (encodedLen < 0) {
          pb->getImage(buf, t);
          os->writeU8(hextileRaw);
          os->writeBytes((const uint8_t*)buf,
                         t.width() * t.height() * sizeof(T));
          oldBgValid = oldFgValid = false;
          continue;
        }
      }

      os->writeU8(tileType);
      if (tileType & hextileBgSpecified) writePixel(os, bg);
      if (tileType & hextileFgSpecified) writePixel(os, fg);
      if (tileType & hextileAnySubrects) os->writeBytes(encoded, encodedLen);
    }
  }
}

template<class T>
int HextileEncoder::hextileEncodeTile(T* data, int w, int h,
                                      int tileType, uint8_t* encoded,
                                      T bg)
{
  uint8_t* nSubrectsPtr = encoded;
  *nSubrectsPtr = 0;
  encoded++;

  for (int y = 0; y < h; y++)
  {
    int x = 0;
    while (x < w) {
      if (*data == bg) {
        x++;
        data++;
        continue;
      }

      // Find horizontal subrect first
      T* ptr = data+1;
      T* eol = data+w-x;
      while (ptr < eol && *ptr == *data) ptr++;
      int sw = ptr - data;

      ptr = data + w;
      int sh = 1;
      while (sh < h-y) {
        eol = ptr + sw;
        while (ptr < eol)
          if (*ptr++ != *data) goto endOfSubrect;
        ptr += w - sw;
        sh++;
      }
    endOfSubrect:

      (*nSubrectsPtr)++;

      if (tileType & hextileSubrectsColoured) {
        if (encoded - nSubrectsPtr + sizeof(T) > w*h*sizeof(T))
          return -1;

        if (sizeof(T) == 1) {
          *encoded++ = *data;
        } else if (sizeof(T) == 2) {
          *encoded++ = ((uint8_t*)data)[0];
          *encoded++ = ((uint8_t*)data)[1];
        } else if (sizeof(T) == 4) {
          *encoded++ = ((uint8_t*)data)[0];
          *encoded++ = ((uint8_t*)data)[1];
          *encoded++ = ((uint8_t*)data)[2];
          *encoded++ = ((uint8_t*)data)[3];
        }
      }

      if ((size_t)(encoded - nSubrectsPtr + 2) > w*h*sizeof(T))
        return -1;
      *encoded++ = (x << 4) | y;
      *encoded++ = ((sw-1) << 4) | (sh-1);

      ptr = data+w;
      T* eor = data+w*sh;
      while (ptr < eor) {
        eol = ptr + sw;
        while (ptr < eol) *ptr++ = bg;
        ptr += w - sw;
      }
      x += sw;
      data += sw;
    }
  }
  return encoded - nSubrectsPtr;
}

template<class T>
int HextileEncoder::testTileType(T* data, int w, int h, T* bg, T* fg)
{
  T pix1 = *data;
  T* end = data + w * h;

  T* ptr = data + 1;
  while (ptr < end && *ptr == pix1)
    ptr++;

  if (ptr == end) {
    *bg = pix1;
    return 0;                   // solid-color tile
  }

  int count1 = ptr - data;
  int count2 = 1;
  T pix2 = *ptr++;
  int tileType = hextileAnySubrects;

  for (; ptr < end; ptr++) {
    if (*ptr == pix1) {
      count1++;
    } else if (*ptr == pix2) {
      count2++;
    } else {
      tileType |= hextileSubrectsColoured;
      break;
    }
  }

  if (count1 >= count2) {
    *bg = pix1; *fg = pix2;
  } else {
    *bg = pix2; *fg = pix1;
  }
  return tileType;
}

//
// This class analyzes a separate tile and encodes its subrectangles.
//

template<class T>
class HextileTile {

 public:

  HextileTile ();

  //
  // Initialize existing object instance with new tile data.
  //
  void newTile(const T *src, int w, int h);

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
  size_t getSize() const { return m_size; }

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
  void encode(uint8_t* dst) const;

 protected:

  //
  // Analyze the tile pixels, fill in all the data fields.
  //
  void analyze();

  const T *m_tile;
  int m_width;
  int m_height;

  size_t m_size;
  int m_flags;
  T m_background;
  T m_foreground;

  int m_numSubrects;
  uint8_t m_coords[256 * 2];
  T m_colors[256];

 private:

  bool m_processed[16][16];
  Palette m_pal;
};

template<class T>
HextileTile<T>::HextileTile()
  : m_tile(nullptr), m_width(0), m_height(0),
    m_size(0), m_flags(0), m_background(0), m_foreground(0),
    m_numSubrects(0)
{
}

template<class T>
void HextileTile<T>::newTile(const T *src, int w, int h)
{
  m_tile = src;
  m_width = w;
  m_height = h;

  analyze();
}

template<class T>
void HextileTile<T>::analyze()
{
  assert(m_tile && m_width && m_height);

  const T *ptr = m_tile;
  const T *end = &m_tile[m_width * m_height];
  T color = *ptr++;
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

  T *colorsPtr = m_colors;
  uint8_t *coordsPtr = m_coords;
  m_pal.clear();
  m_numSubrects = 0;

  // Have we found the first subrect already?
  if (y > 0) {
    *colorsPtr++ = color;
    *coordsPtr++ = 0;
    *coordsPtr++ = (uint8_t)(((m_width - 1) << 4) | ((y - 1) & 0x0F));
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
      *coordsPtr++ = (uint8_t)((x << 4) | (y & 0x0F));
      *coordsPtr++ = (uint8_t)(((sw - 1) << 4) | ((sh - 1) & 0x0F));

      if (!m_pal.insert(color, 1) ||
          ((size_t)m_pal.size() > (48 + 2 * sizeof(T)*8))) {
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

  m_background = (T)m_pal.getColour(0);
  m_flags = hextileAnySubrects;
  int numSubrects = m_numSubrects - m_pal.getCount(0);

  if (numColors == 2) {
    // Monochrome tile
    m_foreground = (T)m_pal.getColour(1);
    m_size = 1 + 2 * numSubrects;
  } else {
    // Colored tile
    m_flags |= hextileSubrectsColoured;
    m_size = 1 + (2 + sizeof(T)) * numSubrects;
  }
}

template<class T>
void HextileTile<T>::encode(uint8_t *dst) const
{
  assert(m_numSubrects && (m_flags & hextileAnySubrects));

  // Zero subrects counter
  uint8_t *numSubrectsPtr = dst;
  *dst++ = 0;

  for (int i = 0; i < m_numSubrects; i++) {
    if (m_colors[i] == m_background)
      continue;

    if (m_flags & hextileSubrectsColoured) {
      if (sizeof(T) == 1) {
        *dst++ = m_colors[i];
      } else if (sizeof(T) == 2) {
        *dst++ = ((uint8_t*)&m_colors[i])[0];
        *dst++ = ((uint8_t*)&m_colors[i])[1];
      } else if (sizeof(T) == 4) {
        *dst++ = ((uint8_t*)&m_colors[i])[0];
        *dst++ = ((uint8_t*)&m_colors[i])[1];
        *dst++ = ((uint8_t*)&m_colors[i])[2];
        *dst++ = ((uint8_t*)&m_colors[i])[3];
      }
    }
    *dst++ = m_coords[i * 2];
    *dst++ = m_coords[i * 2 + 1];

    (*numSubrectsPtr)++;
  }

  assert((size_t)(dst - numSubrectsPtr) == m_size);
}

//
// Main encoding function.
//

template<class T>
void HextileEncoder::hextileEncodeBetter(rdr::OutStream* os,
                                         const PixelBuffer* pb)
{
  Rect t;
  T buf[256];
  T oldBg = 0, oldFg = 0;
  bool oldBgValid = false;
  bool oldFgValid = false;
  uint8_t encoded[256*sizeof(T)];

  HextileTile<T> tile;

  for (t.tl.y = 0; t.tl.y < pb->height(); t.tl.y += 16) {

    t.br.y = __rfbmin(pb->height(), t.tl.y + 16);

    for (t.tl.x = 0; t.tl.x < pb->width(); t.tl.x += 16) {

      t.br.x = __rfbmin(pb->width(), t.tl.x + 16);

      pb->getImage(buf, t);

      tile.newTile(buf, t.width(), t.height());
      int tileType = tile.getFlags();
      size_t encodedLen = tile.getSize();

      if ( (tileType & hextileRaw) != 0 ||
           encodedLen >= t.width() * t.height() * sizeof(T)) {
        os->writeU8(hextileRaw);
        os->writeBytes((const uint8_t*)buf,
                       t.width() * t.height() * sizeof(T));
        oldBgValid = oldFgValid = false;
        continue;
      }

      T bg = tile.getBackground();
      T fg = 0;

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
      if (tileType & hextileBgSpecified) writePixel(os, bg);
      if (tileType & hextileFgSpecified) writePixel(os, fg);
      if (tileType & hextileAnySubrects) os->writeBytes(encoded, encodedLen);
    }
  }
}
