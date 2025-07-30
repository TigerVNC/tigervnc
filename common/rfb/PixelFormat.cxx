/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2009-2022 Pierre Ossman for Cendio AB
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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <core/i18n.h>

#include <rdr/InStream.h>
#include <rdr/OutStream.h>

#include <rfb/Exception.h>
#include <rfb/PixelFormat.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

using namespace rfb;

uint8_t PixelFormat::upconvTable[256*8];
uint8_t PixelFormat::downconvTable[256*8];

class PixelFormat::Init {
public:
  Init();
};

PixelFormat::Init PixelFormat::_init;


PixelFormat::Init::Init()
{
  int bits;

  // Shifting bits is almost perfect, but not quite. And
  // a lookup table is still quicker when there is a large
  // difference between the source and destination depth.

  for (bits = 1;bits <= 8;bits++) {
    int i, maxVal;
    uint8_t *subUpTable;
    uint8_t *subDownTable;

    maxVal = (1 << bits) - 1;
    subUpTable = &upconvTable[(bits-1)*256];
    subDownTable = &downconvTable[(bits-1)*256];

    for (i = 0;i <= maxVal;i++)
      subUpTable[i] = i * 255 / maxVal;

    // Duplicate the up table so that we don't have to care about
    // the upper bits when doing a lookup
    for (;i < 256;i += maxVal+1)
      memcpy(&subUpTable[i], &subUpTable[0], maxVal+1);

    for (i = 0;i <= 255;i++)
      subDownTable[i] = (i * maxVal + 128) / 255;
  }
}


PixelFormat::PixelFormat(int b, int d, bool e, bool t,
                         int rm, int gm, int bm, int rs, int gs, int bs)
  : bpp(b), depth(d), trueColour(t), bigEndian(e),
    redMax(rm), greenMax(gm), blueMax(bm),
    redShift(rs), greenShift(gs), blueShift(bs)
{
  if (!isSane())
    throw std::invalid_argument(_("Invalid pixel format"));

  updateState();
}

PixelFormat::PixelFormat()
  : bpp(8), depth(8), trueColour(true), bigEndian(false),
    redMax(7), greenMax(7), blueMax(3),
    redShift(0), greenShift(3), blueShift(6)
{
  updateState();
}

bool PixelFormat::operator==(const PixelFormat& other) const
{
  if (bpp != other.bpp || depth != other.depth)
    return false;

  if (redMax != other.redMax)
    return false;
  if (greenMax != other.greenMax)
    return false;
  if (blueMax != other.blueMax)
    return false;

  // Endianness requires more care to determine compatibility
  if (bigEndian == other.bigEndian || bpp == 8) {
    if (redShift != other.redShift)
      return false;
    if (greenShift != other.greenShift)
      return false;
    if (blueShift != other.blueShift)
      return false;
  } else {
    // Has to be the same byte for each channel
    if (redShift/8 != (3 - other.redShift/8))
      return false;
    if (greenShift/8 != (3 - other.greenShift/8))
      return false;
    if (blueShift/8 != (3 - other.blueShift/8))
      return false;

    // And the same bit offset within the byte
    if (redShift%8 != other.redShift%8)
      return false;
    if (greenShift%8 != other.greenShift%8)
      return false;
    if (blueShift%8 != other.blueShift%8)
      return false;

    // And not cross a byte boundary
    if (redShift/8 != (redShift + redBits - 1)/8)
      return false;
    if (greenShift/8 != (greenShift + greenBits - 1)/8)
      return false;
    if (blueShift/8 != (blueShift + blueBits - 1)/8)
      return false;
  }

  return true;
}

bool PixelFormat::operator!=(const PixelFormat& other) const
{
  return !(*this == other);
}

void PixelFormat::read(rdr::InStream* is)
{
  bpp = is->readU8();
  depth = is->readU8();
  bigEndian = is->readU8();
  trueColour = is->readU8();
  redMax = is->readU16();
  greenMax = is->readU16();
  blueMax = is->readU16();
  redShift = is->readU8();
  greenShift = is->readU8();
  blueShift = is->readU8();
  is->skip(3);

  // We have no real support for colour maps. If the client
  // wants one, then we force a 8-bit true colour format and
  // pretend it's a colour map.
  if (!trueColour) {
    redMax = 7;
    greenMax = 7;
    blueMax = 3;
    redShift = 0;
    greenShift = 3;
    blueShift = 6;
  }

  if (!isSane())
    throw protocol_error(_("Invalid pixel format"));

  updateState();
}

void PixelFormat::write(rdr::OutStream* os) const
{
  os->writeU8(bpp);
  os->writeU8(depth);
  os->writeU8(bigEndian);
  os->writeU8(trueColour);
  os->writeU16(redMax);
  os->writeU16(greenMax);
  os->writeU16(blueMax);
  os->writeU8(redShift);
  os->writeU8(greenShift);
  os->writeU8(blueShift);
  os->pad(3);
}


bool PixelFormat::is888(void) const
{
  if (!trueColour)
    return false;
  if (bpp != 32)
    return false;
  if (depth != 24)
    return false;
  if (redMax != 255)
    return false;
  if (greenMax != 255)
    return false;
  if (blueMax != 255)
    return false;
  if ((redShift & 0x7) != 0)
    return false;
  if ((greenShift & 0x7) != 0)
    return false;
  if ((blueShift & 0x7) != 0)
    return false;

  return true;
}


bool PixelFormat::isBigEndian(void) const
{
  return bigEndian;
}


bool PixelFormat::isLittleEndian(void) const
{
  return ! bigEndian;
}


void PixelFormat::bufferFromRGB(uint8_t *dst, const uint8_t* src, int pixels) const
{
  bufferFromRGB(dst, src, pixels, pixels, 1);
}

void PixelFormat::bufferFromRGB(uint8_t *dst, const uint8_t* src,
                                int w, int stride, int h) const
{
  if (is888()) {
    // Optimised common case
    uint8_t *r, *g, *b, *x;

    if (bigEndian) {
      r = dst + (24 - redShift)/8;
      g = dst + (24 - greenShift)/8;
      b = dst + (24 - blueShift)/8;
      x = dst + (24 - (48 - redShift - greenShift - blueShift))/8;
    } else {
      r = dst + redShift/8;
      g = dst + greenShift/8;
      b = dst + blueShift/8;
      x = dst + (48 - redShift - greenShift - blueShift)/8;
    }

    int dstPad = (stride - w) * 4;
    while (h--) {
      int w_ = w;
      while (w_--) {
        *r = *(src++);
        *g = *(src++);
        *b = *(src++);
        *x = 0;
        r += 4;
        g += 4;
        b += 4;
        x += 4;
      }
      r += dstPad;
      g += dstPad;
      b += dstPad;
      x += dstPad;
    }
  } else {
    // Generic code
    int dstPad = (stride - w) * bpp/8;
    while (h--) {
      int w_ = w;
      while (w_--) {
        Pixel p;
        uint8_t r, g, b;

        r = *(src++);
        g = *(src++);
        b = *(src++);

        p = pixelFromRGB(r, g, b);

        bufferFromPixel(dst, p);
        dst += bpp/8;
      }
      dst += dstPad;
    }
  }
}


void PixelFormat::rgbFromBuffer(uint8_t* dst, const uint8_t* src, int pixels) const
{
  rgbFromBuffer(dst, src, pixels, pixels, 1);
}


void PixelFormat::rgbFromBuffer(uint8_t* dst, const uint8_t* src,
                                int w, int stride, int h) const
{
  if (is888()) {
    // Optimised common case
    const uint8_t *r, *g, *b;

    if (bigEndian) {
      r = src + (24 - redShift)/8;
      g = src + (24 - greenShift)/8;
      b = src + (24 - blueShift)/8;
    } else {
      r = src + redShift/8;
      g = src + greenShift/8;
      b = src + blueShift/8;
    }

    int srcPad = (stride - w) * 4;
    while (h--) {
      int w_ = w;
      while (w_--) {
        *(dst++) = *r;
        *(dst++) = *g;
        *(dst++) = *b;
        r += 4;
        g += 4;
        b += 4;
      }
      r += srcPad;
      g += srcPad;
      b += srcPad;
    }
  } else {
    // Generic code
    int srcPad = (stride - w) * bpp/8;
    while (h--) {
      int w_ = w;
      while (w_--) {
        Pixel p;
        uint8_t r, g, b;

        p = pixelFromBuffer(src);

        rgbFromPixel(p, &r, &g, &b);

        *(dst++) = r;
        *(dst++) = g;
        *(dst++) = b;
        src += bpp/8;
      }
      src += srcPad;
    }
  }
}


Pixel PixelFormat::pixelFromPixel(const PixelFormat &srcPF, Pixel src) const
{
  uint16_t r, g, b;
  srcPF.rgbFromPixel(src, &r, &g, &b);
  return pixelFromRGB(r, g, b);
}


void PixelFormat::bufferFromBuffer(uint8_t* dst, const PixelFormat &srcPF,
                                   const uint8_t* src, int pixels) const
{
  bufferFromBuffer(dst, srcPF, src, pixels, 1, pixels, pixels);
}

#define IS_ALIGNED(v, a) (((intptr_t)v & (a-1)) == 0)

void PixelFormat::bufferFromBuffer(uint8_t* dst, const PixelFormat &srcPF,
                                   const uint8_t* src, int w, int h,
                                   int dstStride, int srcStride) const
{
  if (*this == srcPF) {
    // Trivial case
    while (h--) {
      memcpy(dst, src, w * bpp/8);
      dst += dstStride * bpp/8;
      src += srcStride * srcPF.bpp/8;
    }
  } else if (is888() && srcPF.is888()) {
    // Optimised common case A: byte shuffling (e.g. endian conversion)
    uint8_t *d[4], *s[4];
    int dstPad, srcPad;

    if (bigEndian) {
      s[0] = dst + (24 - redShift)/8;
      s[1] = dst + (24 - greenShift)/8;
      s[2] = dst + (24 - blueShift)/8;
      s[3] = dst + (24 - (48 - redShift - greenShift - blueShift))/8;
    } else {
      s[0] = dst + redShift/8;
      s[1] = dst + greenShift/8;
      s[2] = dst + blueShift/8;
      s[3] = dst + (48 - redShift - greenShift - blueShift)/8;
    }

    if (srcPF.bigEndian) {
      d[(24 - srcPF.redShift)/8] = s[0];
      d[(24 - srcPF.greenShift)/8] = s[1];
      d[(24 - srcPF.blueShift)/8] = s[2];
      d[(24 - (48 - srcPF.redShift - srcPF.greenShift - srcPF.blueShift))/8] = s[3];
    } else {
      d[srcPF.redShift/8] = s[0];
      d[srcPF.greenShift/8] = s[1];
      d[srcPF.blueShift/8] = s[2];
      d[(48 - srcPF.redShift - srcPF.greenShift - srcPF.blueShift)/8] = s[3];
    }

    dstPad = (dstStride - w) * 4;
    srcPad = (srcStride - w) * 4;
    while (h--) {
      int w_ = w;
      while (w_--) {
        *d[0] = *(src++);
        *d[1] = *(src++);
        *d[2] = *(src++);
        *d[3] = *(src++);
        d[0] += 4;
        d[1] += 4;
        d[2] += 4;
        d[3] += 4;
      }
      d[0] += dstPad;
      d[1] += dstPad;
      d[2] += dstPad;
      d[3] += dstPad;
      src += srcPad;
    }
  } else if (IS_ALIGNED(dst, bpp/8) && srcPF.is888()) {
    // Optimised common case B: 888 source
    switch (bpp) {
    case 8:
      directBufferFromBufferFrom888((uint8_t*)dst, srcPF, src,
                                    w, h, dstStride, srcStride);
      break;
    case 16:
      directBufferFromBufferFrom888((uint16_t*)dst, srcPF, src,
                                    w, h, dstStride, srcStride);
      break;
    case 32:
      directBufferFromBufferFrom888((uint32_t*)dst, srcPF, src,
                                    w, h, dstStride, srcStride);
      break;
    }
  } else if (IS_ALIGNED(src, srcPF.bpp/8) && is888()) {
    // Optimised common case C: 888 destination
    switch (srcPF.bpp) {
    case 8:
      directBufferFromBufferTo888(dst, srcPF, (uint8_t*)src,
                                  w, h, dstStride, srcStride);
      break;
    case 16:
      directBufferFromBufferTo888(dst, srcPF, (uint16_t*)src,
                                  w, h, dstStride, srcStride);
      break;
    case 32:
      directBufferFromBufferTo888(dst, srcPF, (uint32_t*)src,
                                  w, h, dstStride, srcStride);
      break;
    }
  } else {
    // Generic code
    int dstPad = (dstStride - w) * bpp/8;
    int srcPad = (srcStride - w) * srcPF.bpp/8;
    while (h--) {
      int w_ = w;
      while (w_--) {
        Pixel p;
        uint8_t r, g, b;

        p = srcPF.pixelFromBuffer(src);
        srcPF.rgbFromPixel(p, &r, &g, &b);
        p = pixelFromRGB(r, g, b);
        bufferFromPixel(dst, p);

        dst += bpp/8;
        src += srcPF.bpp/8;
      }
      dst += dstPad;
      src += srcPad;
    }
  }
}


void PixelFormat::print(char* str, int len) const
{
  // Unfortunately snprintf is not widely available so we build the string up
  // using strncat - not pretty, but should be safe against buffer overruns.

  char num[20];
  if (len < 1) return;
  str[0] = 0;
  strncat(str, "depth ", len-1-strlen(str));
  sprintf(num,"%d",depth);
  strncat(str, num, len-1-strlen(str));
  strncat(str, " (", len-1-strlen(str));
  sprintf(num,"%d",bpp);
  strncat(str, num, len-1-strlen(str));
  strncat(str, "bpp)", len-1-strlen(str));
  if (bpp != 8) {
    if (bigEndian)
      strncat(str, " big-endian", len-1-strlen(str));
    else
      strncat(str, " little-endian", len-1-strlen(str));
  }

  if (!trueColour) {
    strncat(str, " color-map", len-1-strlen(str));
    return;
  }

  if (blueShift == 0 && greenShift > blueShift && redShift > greenShift &&
      blueMax  == (1 << greenShift) - 1 &&
      greenMax == (1 << (redShift-greenShift)) - 1 &&
      redMax   == (1 << (depth-redShift)) - 1)
  {
    strncat(str, " rgb", len-1-strlen(str));
    sprintf(num,"%d",depth-redShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",redShift-greenShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",greenShift);
    strncat(str, num, len-1-strlen(str));
    return;
  }

  if (redShift == 0 && greenShift > redShift && blueShift > greenShift &&
      redMax   == (1 << greenShift) - 1 &&
      greenMax == (1 << (blueShift-greenShift)) - 1 &&
      blueMax  == (1 << (depth-blueShift)) - 1)
  {
    strncat(str, " bgr", len-1-strlen(str));
    sprintf(num,"%d",depth-blueShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",blueShift-greenShift);
    strncat(str, num, len-1-strlen(str));
    sprintf(num,"%d",greenShift);
    strncat(str, num, len-1-strlen(str));
    return;
  }

  strncat(str, " rgb max ", len-1-strlen(str));
  sprintf(num,"%d,",redMax);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d,",greenMax);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d",blueMax);
  strncat(str, num, len-1-strlen(str));
  strncat(str, " shift ", len-1-strlen(str));
  sprintf(num,"%d,",redShift);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d,",greenShift);
  strncat(str, num, len-1-strlen(str));
  sprintf(num,"%d",blueShift);
  strncat(str, num, len-1-strlen(str));
}


bool PixelFormat::parse(const char* str)
{
  char rgbbgr[4];
  int bits1, bits2, bits3;
  if (sscanf(str, "%3s%1d%1d%1d", rgbbgr, &bits1, &bits2, &bits3) < 4)
    return false;
  
  depth = bits1 + bits2 + bits3;
  bpp = depth <= 8 ? 8 : ((depth <= 16) ? 16 : 32);
  trueColour = true;
  uint32_t endianTest = 1;
  bigEndian = (*(uint8_t*)&endianTest == 0);

  greenShift = bits3;
  greenMax = (1 << bits2) - 1;

  if (strcasecmp(rgbbgr, "bgr") == 0) {
    redShift = 0;
    redMax = (1 << bits3) - 1;
    blueShift = bits3 + bits2;
    blueMax = (1 << bits1) - 1;
  } else if (strcasecmp(rgbbgr, "rgb") == 0) {
    blueShift = 0;
    blueMax = (1 << bits3) - 1;
    redShift = bits3 + bits2;
    redMax = (1 << bits1) - 1;
  } else {
    return false;
  }

  assert(isSane());

  updateState();

  return true;
}


static int bits(uint16_t value)
{
  int bits;

  bits = 16;

  if (!(value & 0xff00)) {
    bits -= 8;
    value <<= 8;
  }
  if (!(value & 0xf000)) {
    bits -= 4;
    value <<= 4;
  }
  if (!(value & 0xc000)) {
    bits -= 2;
    value <<= 2;
  }
  if (!(value & 0x8000)) {
    bits -= 1;
    value <<= 1;
  }

  return bits;
}

void PixelFormat::updateState(void)
{
  int endianTest = 1;

  redBits = bits(redMax);
  greenBits = bits(greenMax);
  blueBits = bits(blueMax);

  maxBits = redBits;
  if (greenBits > maxBits)
    maxBits = greenBits;
  if (blueBits > maxBits)
    maxBits = blueBits;

  minBits = redBits;
  if (greenBits < minBits)
    minBits = greenBits;
  if (blueBits < minBits)
    minBits = blueBits;

  if (((*(char*)&endianTest) == 0) != bigEndian)
    endianMismatch = true;
  else
    endianMismatch = false;
}

bool PixelFormat::isSane(void)
{
  int totalBits;

  if ((bpp != 8) && (bpp != 16) && (bpp != 32))
    return false;
  if (depth > bpp)
    return false;

  if (!trueColour && (depth != 8))
    return false;

  if ((redMax & (redMax + 1)) != 0)
    return false;
  if ((greenMax & (greenMax + 1)) != 0)
    return false;
  if ((blueMax & (blueMax + 1)) != 0)
    return false;

  /*
   * We don't allow individual channels > 8 bits in order to keep our
   * conversions simple.
   */
  if (redMax >= (1 << 8))
    return false;
  if (greenMax >= (1 << 8))
    return false;
  if (blueMax >= (1 << 8))
    return false;

  totalBits = bits(redMax) + bits(greenMax) + bits(blueMax);
  if (totalBits > depth)
    return false;

  if ((bits(redMax) + redShift) > bpp)
    return false;
  if ((bits(greenMax) + greenShift) > bpp)
    return false;
  if ((bits(blueMax) + blueShift) > bpp)
    return false;

  if (((redMax << redShift) & (greenMax << greenShift)) != 0)
    return false;
  if (((redMax << redShift) & (blueMax << blueShift)) != 0)
    return false;
  if (((greenMax << greenShift) & (blueMax << blueShift)) != 0)
    return false;

  return true;
}

static inline uint8_t swap(uint8_t n)
{
  return n;
}

static inline uint16_t swap(uint16_t n)
{
  return (((n) & 0xff) << 8) | (((n) >> 8) & 0xff);
}

static inline uint32_t swap(uint32_t n)
{
  return ((n) >> 24) | (((n) & 0x00ff0000) >> 8) |
         (((n) & 0x0000ff00) << 8) | ((n) << 24);
}

template<class T>
void PixelFormat::directBufferFromBufferFrom888(T* dst,
                                                const PixelFormat &srcPF,
                                                const uint8_t* src,
                                                int w, int h,
                                                int dstStride,
                                                int srcStride) const
{
  const uint8_t *r, *g, *b;
  int dstPad, srcPad;

  const uint8_t *redDownTable, *greenDownTable, *blueDownTable;

  redDownTable = &downconvTable[(redBits-1)*256];
  greenDownTable = &downconvTable[(greenBits-1)*256];
  blueDownTable = &downconvTable[(blueBits-1)*256];

  if (srcPF.bigEndian) {
    r = src + (24 - srcPF.redShift)/8;
    g = src + (24 - srcPF.greenShift)/8;
    b = src + (24 - srcPF.blueShift)/8;
  } else {
    r = src + srcPF.redShift/8;
    g = src + srcPF.greenShift/8;
    b = src + srcPF.blueShift/8;
  }

  dstPad = (dstStride - w);
  srcPad = (srcStride - w) * 4;
  while (h--) {
    int w_ = w;
    while (w_--) {
      T d;

      d = redDownTable[*r] << redShift;
      d |= greenDownTable[*g] << greenShift;
      d |= blueDownTable[*b] << blueShift;

      if (endianMismatch)
        d = swap(d);

      *dst = d;

      dst++;
      r += 4;
      g += 4;
      b += 4;
    }
    dst += dstPad;
    r += srcPad;
    g += srcPad;
    b += srcPad;
  }
}

template<class T>
void PixelFormat::directBufferFromBufferTo888(uint8_t* dst,
                                              const PixelFormat &srcPF,
                                              const T* src,
                                              int w, int h,
                                              int dstStride,
                                              int srcStride) const
{
  uint8_t *r, *g, *b, *x;
  int dstPad, srcPad;

  const uint8_t *redUpTable, *greenUpTable, *blueUpTable;

  redUpTable = &upconvTable[(srcPF.redBits-1)*256];
  greenUpTable = &upconvTable[(srcPF.greenBits-1)*256];
  blueUpTable = &upconvTable[(srcPF.blueBits-1)*256];

  if (bigEndian) {
    r = dst + (24 - redShift)/8;
    g = dst + (24 - greenShift)/8;
    b = dst + (24 - blueShift)/8;
    x = dst + (24 - (48 - redShift - greenShift - blueShift))/8;
  } else {
    r = dst + redShift/8;
    g = dst + greenShift/8;
    b = dst + blueShift/8;
    x = dst + (48 - redShift - greenShift - blueShift)/8;
  }

  dstPad = (dstStride - w) * 4;
  srcPad = (srcStride - w);
  while (h--) {
    int w_ = w;
    while (w_--) {
      T s;

      s = *src;

      if (srcPF.endianMismatch)
        s = swap(s);

      *r = redUpTable[(s >> srcPF.redShift) & 0xff];
      *g = greenUpTable[(s >> srcPF.greenShift) & 0xff];
      *b = blueUpTable[(s >> srcPF.blueShift) & 0xff];
      *x = 0;

      r += 4;
      g += 4;
      b += 4;
      x += 4;
      src++;
    }
    r += dstPad;
    g += dstPad;
    b += dstPad;
    x += dstPad;
    src += srcPad;
  }
}
