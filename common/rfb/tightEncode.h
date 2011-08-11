/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
// tightEncode.h - Tight encoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// GET_IMAGE_INTO_BUF - gets a rectangle of pixel data into a buffer
//

#include <rdr/OutStream.h>
#include <rdr/ZlibOutStream.h>
#include <assert.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define WRITE_PIXEL CONCAT2E(writeOpaque,BPP)
#define TIGHT_ENCODE CONCAT2E(tightEncode,BPP)
#define SWAP_PIXEL CONCAT2E(SWAP,BPP)
#define HASH_FUNCTION CONCAT2E(HASH_FUNC,BPP)
#define PACK_PIXELS CONCAT2E(packPixels,BPP)
#define ENCODE_SOLID_RECT CONCAT2E(encodeSolidRect,BPP)
#define ENCODE_FULLCOLOR_RECT CONCAT2E(encodeFullColorRect,BPP)
#define ENCODE_MONO_RECT CONCAT2E(encodeMonoRect,BPP)
#define ENCODE_INDEXED_RECT CONCAT2E(encodeIndexedRect,BPP)
#define ENCODE_JPEG_RECT CONCAT2E(encodeJpegRect,BPP)
#define FILL_PALETTE CONCAT2E(fillPalette,BPP)
#define CHECK_SOLID_TILE CONCAT2E(checkSolidTile,BPP)

#ifndef TIGHT_ONCE
#define TIGHT_ONCE

//
// C-style structures to store palette entries and compression paramentes.
// Such code probably should be converted into C++ classes.
//

struct TIGHT_COLOR_LIST {
  TIGHT_COLOR_LIST *next;
  int idx;
  rdr::U32 rgb;
};

struct TIGHT_PALETTE_ENTRY {
  TIGHT_COLOR_LIST *listNode;
  int numPixels;
};

struct TIGHT_PALETTE {
  TIGHT_PALETTE_ENTRY entry[256];
  TIGHT_COLOR_LIST *hash[256];
  TIGHT_COLOR_LIST list[256];
};

// FIXME: Is it really a good idea to use static variables for this?
static bool s_pack24;             // use 24-bit packing for 32-bit pixels

// FIXME: Make a separate class for palette operations.
static int s_palMaxColors, s_palNumColors;
static rdr::U32 s_monoBackground, s_monoForeground;
static TIGHT_PALETTE s_palette;

//
// Swapping bytes in pixels.
// FIXME: Use a sort of ImageGetter that does not convert pixel format?
//

#ifndef SWAP16
#define SWAP16(n) ((((n) & 0xff) << 8) | (((n) >> 8) & 0xff))
#endif
#ifndef SWAP32
#define SWAP32(n) (((n) >> 24) | (((n) & 0x00ff0000) >> 8) | \
                   (((n) & 0x0000ff00) << 8) | ((n) << 24))
#endif

//
// Functions to operate on palette structures.
//

#define HASH_FUNC16(rgb) ((int)(((rgb >> 8) + rgb) & 0xFF))
#define HASH_FUNC32(rgb) ((int)(((rgb >> 16) + (rgb >> 8)) & 0xFF))

static void paletteReset(void)
{
  s_palNumColors = 0;
  memset(s_palette.hash, 0, 256 * sizeof(TIGHT_COLOR_LIST *));
}

static int paletteInsert(rdr::U32 rgb, int numPixels, int bpp)
{
  TIGHT_COLOR_LIST *pnode;
  TIGHT_COLOR_LIST *prev_pnode = NULL;
  int hash_key, idx, new_idx, count;

  hash_key = (bpp == 16) ? HASH_FUNC16(rgb) : HASH_FUNC32(rgb);

  pnode = s_palette.hash[hash_key];

  while (pnode != NULL) {
    if (pnode->rgb == rgb) {
      // Such palette entry already exists.
      new_idx = idx = pnode->idx;
      count = s_palette.entry[idx].numPixels + numPixels;
      if (new_idx && s_palette.entry[new_idx-1].numPixels < count) {
        do {
          s_palette.entry[new_idx] = s_palette.entry[new_idx-1];
          s_palette.entry[new_idx].listNode->idx = new_idx;
          new_idx--;
        }
        while (new_idx &&
          s_palette.entry[new_idx-1].numPixels < count);
        s_palette.entry[new_idx].listNode = pnode;
        pnode->idx = new_idx;
      }
      s_palette.entry[new_idx].numPixels = count;
      return s_palNumColors;
    }
    prev_pnode = pnode;
    pnode = pnode->next;
  }

  // Check if palette is full.
  if ( s_palNumColors == 256 || s_palNumColors == s_palMaxColors ) {
    s_palNumColors = 0;
    return 0;
  }

  // Move palette entries with lesser pixel counts.
  for ( idx = s_palNumColors;
  idx > 0 && s_palette.entry[idx-1].numPixels < numPixels;
  idx-- ) {
    s_palette.entry[idx] = s_palette.entry[idx-1];
    s_palette.entry[idx].listNode->idx = idx;
  }

  // Add new palette entry into the freed slot.
  pnode = &s_palette.list[s_palNumColors];
  if (prev_pnode != NULL) {
    prev_pnode->next = pnode;
  } else {
    s_palette.hash[hash_key] = pnode;
  }
  pnode->next = NULL;
  pnode->idx = idx;
  pnode->rgb = rgb;
  s_palette.entry[idx].listNode = pnode;
  s_palette.entry[idx].numPixels = numPixels;

  return (++s_palNumColors);
}

//
// Compress the data (but do not perform actual compression if the data
// size is less than TIGHT_MIN_TO_COMPRESS bytes.
//

static void compressData(rdr::OutStream *os, rdr::ZlibOutStream *zos,
                         const void *buf, const PixelFormat& pf,
                         unsigned int length, int zlibLevel)
{
  if (length < TIGHT_MIN_TO_COMPRESS) {
    os->writeBytes(buf, length);
  } else {
    // FIXME: Using a temporary MemOutStream may be not efficient.
    //        Maybe use the same static object used in the JPEG coder?
    int maxBeforeSize = s_pconf->maxRectSize * (pf.bpp / 8);
    int maxAfterSize = maxBeforeSize + (maxBeforeSize + 99) / 100 + 12;
    rdr::MemOutStream mem_os(maxAfterSize);
    zos->setUnderlying(&mem_os);
    zos->setCompressionLevel(zlibLevel);
    zos->writeBytes(buf, length);
    zos->flush();
    zos->setUnderlying(NULL);
    os->writeCompactLength(mem_os.length());
    os->writeBytes(mem_os.data(), mem_os.length());
  }
}

#endif  // #ifndef TIGHT_ONCE

static void ENCODE_SOLID_RECT     (rdr::OutStream *os,
                                   PIXEL_T *buf, const PixelFormat& pf);
static void ENCODE_FULLCOLOR_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const PixelFormat& pf, const Rect& r);
static void ENCODE_MONO_RECT      (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const PixelFormat& pf, const Rect& r);
#if (BPP != 8)
static void ENCODE_INDEXED_RECT   (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const PixelFormat& pf, const Rect& r);
static void ENCODE_JPEG_RECT      (rdr::OutStream *os, JpegCompressor& jc,
                                   PIXEL_T *buf, const PixelFormat& pf, const Rect& r);
#endif

static void FILL_PALETTE (PIXEL_T *data, int count);

//
// Convert 32-bit color samples into 24-bit colors, in place.
// Performs packing only when redMax, greenMax and blueMax are all 255.
// Color components are assumed to be byte-aligned.
//

static inline unsigned int PACK_PIXELS (PIXEL_T *buf, unsigned int count,
                                        const PixelFormat& pf)
{
#if (BPP != 32)
  return count * sizeof(PIXEL_T);
#else
  if (!s_pack24)
    return count * sizeof(PIXEL_T);

  rdr::U32 pix;
  rdr::U8 *dst = (rdr::U8 *)buf;
  for (unsigned int i = 0; i < count; i++) {
    pix = *buf++;
    pf.rgbFromBuffer(dst, (rdr::U8*)&pix, 1, NULL);
    dst += 3;
  }
  return count * 3;
#endif
}

//
// Main function of the Tight encoder
//

void TIGHT_ENCODE (const Rect& r, rdr::OutStream *os,
                  rdr::ZlibOutStream zos[4], JpegCompressor &jc, void* buf,
                  ConnParams* cp
#ifdef EXTRA_ARGS
                  , EXTRA_ARGS,
#endif
                  bool forceSolid)
{
  const PixelFormat& pf = cp->pf();
  if(forceSolid) {
    GET_IMAGE_INTO_BUF(Rect(r.tl.x, r.tl.y, r.tl.x + 1, r.tl.y + 1), buf);
  }
  else {
    GET_IMAGE_INTO_BUF(r, buf);
  }
  PIXEL_T* pixels = (PIXEL_T*)buf;

#if (BPP == 32)
  // Check if it's necessary to pack 24-bit pixels, and
  // compute appropriate shift values if necessary.
  s_pack24 = pf.is888();
#endif

  if (forceSolid)
    s_palNumColors = 1;
  else {
    s_palMaxColors = r.area() / s_pconf->idxMaxColorsDivisor;
    if (s_pjconf != NULL) s_palMaxColors = s_pconf->palMaxColorsWithJPEG;
    if (s_palMaxColors < 2 && r.area() >= s_pconf->monoMinRectSize) {
      s_palMaxColors = 2;
    }

    FILL_PALETTE(pixels, r.area());
  }

  switch (s_palNumColors) {
  case 0:
    // Truecolor image
#if (BPP != 8)
    if (s_pjconf != NULL) {
      ENCODE_JPEG_RECT(os, jc, pixels, pf, r);
      break;
    }
#endif
    ENCODE_FULLCOLOR_RECT(os, zos, pixels, pf, r);
    break;
  case 1:
    // Solid rectangle
    ENCODE_SOLID_RECT(os, pixels, pf);
    break;
  case 2:
    // Two-color rectangle
    ENCODE_MONO_RECT(os, zos, pixels, pf, r);
    break;
#if (BPP != 8)
  default:
    // Up to 256 different colors
    ENCODE_INDEXED_RECT(os, zos, pixels, pf, r);
#endif
  }
}

//
// Subencoding implementations.
//

static void ENCODE_SOLID_RECT (rdr::OutStream *os, PIXEL_T *buf, const PixelFormat& pf)
{
  os->writeU8(0x08 << 4);

  int length = PACK_PIXELS(buf, 1, pf);
  os->writeBytes(buf, length);
}

static void ENCODE_FULLCOLOR_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const PixelFormat& pf, const Rect& r)
{
  const int streamId = 0;
  os->writeU8(streamId << 4);

  int length = PACK_PIXELS(buf, r.area(), pf);
  compressData(os, &zos[streamId], buf, pf, length, s_pconf->rawZlibLevel);
}

static void ENCODE_MONO_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                              PIXEL_T *buf, const PixelFormat& pf, const Rect& r)
{
  const int streamId = 1;
  os->writeU8((streamId | 0x04) << 4);
  os->writeU8(0x01);

  // Write the palette
  PIXEL_T pal[2] = { (PIXEL_T)s_monoBackground, (PIXEL_T)s_monoForeground };
  os->writeU8(1);
  os->writeBytes(pal, PACK_PIXELS(pal, 2, pf));

  // Encode the data in-place
  PIXEL_T *src = buf;
  rdr::U8 *dst = (rdr::U8 *)buf;
  int w = r.width();
  int h = r.height();
  PIXEL_T bg;
  unsigned int value, mask;
  int aligned_width;
  int x, y, bg_bits;

  bg = (PIXEL_T) s_monoBackground;
  aligned_width = w - w % 8;

  for (y = 0; y < h; y++) {
    for (x = 0; x < aligned_width; x += 8) {
      for (bg_bits = 0; bg_bits < 8; bg_bits++) {
        if (*src++ != bg)
          break;
      }
      if (bg_bits == 8) {
        *dst++ = 0;
        continue;
      }
      mask = 0x80 >> bg_bits;
      value = mask;
      for (bg_bits++; bg_bits < 8; bg_bits++) {
        mask >>= 1;
        if (*src++ != bg) {
          value |= mask;
        }
      }
      *dst++ = (rdr::U8)value;
    }

    mask = 0x80;
    value = 0;
    if (x >= w)
      continue;

    for (; x < w; x++) {
      if (*src++ != bg) {
        value |= mask;
      }
      mask >>= 1;
    }
    *dst++ = (rdr::U8)value;
  }

  // Write the data
  int length = (w + 7) / 8;
  length *= h;
  compressData(os, &zos[streamId], buf, pf, length, s_pconf->monoZlibLevel);
}

#if (BPP != 8)
static void ENCODE_INDEXED_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                 PIXEL_T *buf, const PixelFormat& pf, const Rect& r)
{
  const int streamId = 2;
  os->writeU8((streamId | 0x04) << 4);
  os->writeU8(0x01);

  // Write the palette
  {
    PIXEL_T pal[256];
    for (int i = 0; i < s_palNumColors; i++)
      pal[i] = (PIXEL_T)s_palette.entry[i].listNode->rgb;
    os->writeU8((rdr::U8)(s_palNumColors - 1));
    os->writeBytes(pal, PACK_PIXELS(pal, s_palNumColors, pf));
  }

  // Encode data in-place
  PIXEL_T *src = buf;
  rdr::U8 *dst = (rdr::U8 *)buf;
  int count = r.area();
  PIXEL_T rgb;
  TIGHT_COLOR_LIST *pnode;
  int rep = 0;

  while (count--) {
    rgb = *src++;
    while (count && *src == rgb) {
      rep++, src++, count--;
    }
    pnode = s_palette.hash[HASH_FUNCTION(rgb)];
    while (pnode != NULL) {
      if ((PIXEL_T)pnode->rgb == rgb) {
        *dst++ = (rdr::U8)pnode->idx;
        while (rep) {
          *dst++ = (rdr::U8)pnode->idx;
          rep--;
        }
        break;
      }
      pnode = pnode->next;
    }
  }

  // Write the data
  compressData(os, &zos[streamId], buf, pf, r.area(), s_pconf->idxZlibLevel);
}
#endif  // #if (BPP != 8)

//
// JPEG compression.
//

#if (BPP != 8)
static void ENCODE_JPEG_RECT (rdr::OutStream *os, JpegCompressor& jc,
                              PIXEL_T *buf, const PixelFormat& pf,
                              const Rect& r)
{
  jc.clear();
  jc.compress((rdr::U8 *)buf, r, pf, s_pjconf->jpegQuality,
    s_pjconf->jpegSubSample);
  os->writeU8(0x09 << 4);
  os->writeCompactLength(jc.length());
  os->writeBytes(jc.data(), jc.length());
}
#endif  // #if (BPP != 8)

//
// Determine the number of colors in the rectangle, and fill in the palette.
//

#if (BPP == 8)
static void FILL_PALETTE (PIXEL_T *data, int count)
{
  PIXEL_T c0, c1;
  int i, n0, n1;

  s_palNumColors = 0;

  c0 = data[0];
  for (i = 1; i < count && data[i] == c0; i++);
  if (i == count) {
    s_palNumColors = 1;
    return;                       // Solid rectangle
  }

  if (s_palMaxColors < 2)
    return;

  n0 = i;
  c1 = data[i];
  n1 = 0;
  for (i++; i < count; i++) {
    if (data[i] == c0) {
      n0++;
    } else if (data[i] == c1) {
      n1++;
    } else
      break;
  }
  if (i == count) {
    if (n0 > n1) {
      s_monoBackground = (rdr::U32)c0;
      s_monoForeground = (rdr::U32)c1;
    } else {
      s_monoBackground = (rdr::U32)c1;
      s_monoForeground = (rdr::U32)c0;
    }
    s_palNumColors = 2;           // Two colors
  }
}
#else   // (BPP != 8)
static void FILL_PALETTE (PIXEL_T *data, int count)
{
  PIXEL_T c0, c1, ci = 0;
  int i, n0, n1, ni;

  c0 = data[0];
  for (i = 1; i < count && data[i] == c0; i++);
  if (i >= count) {
    s_palNumColors = 1;           // Solid rectangle
    return;
  }

  if (s_palMaxColors < 2) {
    s_palNumColors = 0;           // Full-color format preferred
    return;
  }

  n0 = i;
  c1 = data[i];
  n1 = 0;
  for (i++; i < count; i++) {
    ci = data[i];
    if (ci == c0) {
      n0++;
    } else if (ci == c1) {
      n1++;
    } else
      break;
  }
  if (i >= count) {
    if (n0 > n1) {
      s_monoBackground = (rdr::U32)c0;
      s_monoForeground = (rdr::U32)c1;
    } else {
      s_monoBackground = (rdr::U32)c1;
      s_monoForeground = (rdr::U32)c0;
    }
    s_palNumColors = 2;           // Two colors
    return;
  }

  paletteReset();
  paletteInsert (c0, (rdr::U32)n0, BPP);
  paletteInsert (c1, (rdr::U32)n1, BPP);

  ni = 1;
  for (i++; i < count; i++) {
    if (data[i] == ci) {
      ni++;
    } else {
      if (!paletteInsert (ci, (rdr::U32)ni, BPP))
        return;
      ci = data[i];
      ni = 1;
    }
  }
  paletteInsert (ci, (rdr::U32)ni, BPP);
}
#endif  // #if (BPP == 8)

bool CHECK_SOLID_TILE(Rect& r, ImageGetter* ig, SMsgWriter* writer,
                      rdr::U32 *colorPtr, bool needSameColor)
{
  PIXEL_T *buf;
  PIXEL_T colorValue;
  int dx, dy;
  Rect sr;

  buf = (PIXEL_T *)writer->getImageBuf(r.area());
  sr.setXYWH(r.tl.x, r.tl.y, 1, 1);
  GET_IMAGE_INTO_BUF(sr, buf);

  colorValue = *buf;
  if (needSameColor && (rdr::U32)colorValue != *colorPtr)
    return false;

  for (dy = 0; dy < r.height(); dy++) {
    Rect sr;
    sr.setXYWH(r.tl.x, r.tl.y + dy, r.width(), 1);
    GET_IMAGE_INTO_BUF(sr, buf);
    for (dx = 0; dx < r.width(); dx++) {
      if (colorValue != buf[dx])
        return false;
    }
  }

  *colorPtr = (rdr::U32)colorValue;
  return true;
}

#undef PIXEL_T
#undef WRITE_PIXEL
#undef TIGHT_ENCODE
#undef SWAP_PIXEL
#undef HASH_FUNCTION
#undef PACK_PIXELS
#undef ENCODE_SOLID_RECT
#undef ENCODE_FULLCOLOR_RECT
#undef ENCODE_MONO_RECT
#undef ENCODE_INDEXED_RECT
#undef ENCODE_JPEG_RECT
#undef FILL_PALETTE
#undef CHECK_SOLID_TILE
}
