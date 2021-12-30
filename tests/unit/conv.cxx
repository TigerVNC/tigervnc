/* Copyright 2013-2014 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rfb/PixelFormat.h>

static const rdr::U8 pixelRed = 0xf1;
static const rdr::U8 pixelGreen = 0xc3;
static const rdr::U8 pixelBlue = 0x97;

static const int fbWidth = 40;
static const int fbHeight = 30;
static const int fbArea = fbWidth * fbHeight;
// Maximum bpp, plus some room for unaligned fudging
static const int fbMalloc = (fbArea * 4) + 4;

typedef bool (*testfn) (const rfb::PixelFormat&, const rfb::PixelFormat&);

struct TestEntry {
  const char *label;
  testfn fn;
};

#define min(a,b) (((a) < (b)) ? (a) : (b))

namespace rfb {

void makePixel(const rfb::PixelFormat &pf,
               rdr::U8 *buffer)
{
  rfb::Pixel p;

  p = 0;
  p |= (pixelRed >> (8 - pf.redBits)) << pf.redShift;
  p |= (pixelGreen >> (8 - pf.greenBits)) << pf.greenShift;
  p |= (pixelBlue >> (8 - pf.blueBits)) << pf.blueShift;

  // FIXME: Should we reimplement this as well?
  pf.bufferFromPixel(buffer, p);
}

bool verifyPixel(const rfb::PixelFormat &dstpf,
                 const rfb::PixelFormat &srcpf,
                 const rdr::U8 *buffer)
{
  rfb::Pixel p;
  int r, g, b;
  int re, ge, be;

  // FIXME: Should we reimplement this as well?
  p = dstpf.pixelFromBuffer(buffer);

  r = (p >> dstpf.redShift) & dstpf.redMax;
  g = (p >> dstpf.greenShift) & dstpf.greenMax;
  b = (p >> dstpf.blueShift) & dstpf.blueMax;

  r = (r * 255 + dstpf.redMax/2) / dstpf.redMax;
  g = (g * 255 + dstpf.greenMax/2) / dstpf.greenMax;
  b = (b * 255 + dstpf.blueMax/2) / dstpf.blueMax;

  // The allowed error depends on:
  //
  //  a) The number of bits the format can hold
  //  b) The number of bits the source format could hold

  re = (1 << (8 - min(dstpf.redBits, srcpf.redBits))) - 1;
  ge = (1 << (8 - min(dstpf.greenBits, srcpf.greenBits))) - 1;
  be = (1 << (8 - min(dstpf.blueBits, srcpf.blueBits))) - 1;

  if (abs(r - pixelRed) > re)
    return false;
  if (abs(g - pixelGreen) > ge)
    return false;
  if (abs(b - pixelBlue) > be)
    return false;

  return true;
}

}

using rfb::makePixel;
using rfb::verifyPixel;

static bool testPixel(const rfb::PixelFormat &dstpf,
                      const rfb::PixelFormat &srcpf)
{
  rfb::Pixel p;
  rdr::U8 buffer[4];

  makePixel(srcpf, buffer);

  p = srcpf.pixelFromBuffer(buffer);
  p = dstpf.pixelFromPixel(srcpf, p);
  memset(buffer, 0, sizeof(buffer));
  dstpf.bufferFromPixel(buffer, p);

  if (!verifyPixel(dstpf, srcpf, buffer))
    return false;

  return true;
}

static bool testBuffer(const rfb::PixelFormat &dstpf,
                       const rfb::PixelFormat &srcpf)
{
  int i, x, y, unaligned;
  rdr::U8 bufIn[fbMalloc], bufOut[fbMalloc];

  // Once aligned, and once unaligned
  for (unaligned = 0;unaligned < 2;unaligned++) {
    for (i = 0;i < fbArea;i++)
      makePixel(srcpf, bufIn + unaligned + i*srcpf.bpp/8);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromBuffer(bufOut + unaligned, srcpf,
                           bufIn + unaligned, fbArea);

    for (i = 0;i < fbArea;i++) {
      if (!verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8))
        return false;
    }

    memset(bufIn, 0, sizeof(bufIn));
    for (y = 0;y < fbHeight;y++) {
      for (x = 0;x < fbWidth/2;x++)
        makePixel(srcpf, bufIn + unaligned + (x + y*fbWidth)*srcpf.bpp/8);
    }

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromBuffer(bufOut + unaligned, srcpf, bufIn + unaligned,
                           fbWidth/2, fbHeight, fbWidth, fbWidth);

    for (y = 0;y < fbHeight;y++) {
      for (x = 0;x < fbWidth;x++) {
        if (x < fbWidth/2) {
          if (!verifyPixel(dstpf, srcpf,
                           bufOut + unaligned + (x + y*fbWidth)*dstpf.bpp/8))
            return false;
        } else {
          const rdr::U8 zero[4] = { 0, 0, 0, 0 };
          if (memcmp(bufOut + unaligned + (x + y*fbWidth)*dstpf.bpp/8, zero,
                     dstpf.bpp/8) != 0)
            return false;
        }
      }
    }
  }

  return true;
}

static bool testRGB(const rfb::PixelFormat &dstpf,
                    const rfb::PixelFormat &srcpf)
{
  int i, x, y, unaligned;
  rdr::U8 bufIn[fbMalloc], bufRGB[fbMalloc], bufOut[fbMalloc];

  // Once aligned, and once unaligned
  for (unaligned = 0;unaligned < 2;unaligned++) {
    for (i = 0;i < fbArea;i++)
      makePixel(srcpf, bufIn + unaligned + i*srcpf.bpp/8);

    memset(bufRGB, 0, sizeof(bufRGB));
    srcpf.rgbFromBuffer(bufRGB + unaligned, bufIn + unaligned, fbArea);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromRGB(bufOut + unaligned, bufRGB + unaligned, fbArea);

    for (i = 0;i < fbArea;i++) {
      if (!verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8))
        return false;
    }

    memset(bufIn, 0, sizeof(bufIn));
    for (y = 0;y < fbHeight;y++) {
      for (x = 0;x < fbWidth/2;x++)
        makePixel(srcpf, bufIn + unaligned + (x + y*fbWidth)*srcpf.bpp/8);
    }

    memset(bufRGB, 0, sizeof(bufRGB));
    srcpf.rgbFromBuffer(bufRGB + unaligned, bufIn + unaligned,
                        fbWidth/2, fbWidth, fbHeight);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromRGB(bufOut + unaligned, bufRGB + unaligned,
                        fbWidth/2, fbWidth, fbHeight);

    for (y = 0;y < fbHeight;y++) {
      for (x = 0;x < fbWidth;x++) {
        if (x < fbWidth/2) {
          if (!verifyPixel(dstpf, srcpf,
                           bufOut + unaligned + (x + y*fbWidth)*dstpf.bpp/8))
            return false;
        } else {
          const rdr::U8 zero[4] = { 0, 0, 0, 0 };
          if (memcmp(bufOut + unaligned + (x + y*fbWidth)*dstpf.bpp/8, zero,
                     dstpf.bpp/8) != 0)
            return false;
        }
      }
    }
  }

  return true;
}

static bool testPixelRGB(const rfb::PixelFormat &dstpf,
                         const rfb::PixelFormat &srcpf)
{
  rfb::Pixel p;
  rdr::U16 r16, g16, b16;
  rdr::U8 r8, g8, b8;
  rdr::U8 buffer[4];

  makePixel(srcpf, buffer);

  p = srcpf.pixelFromBuffer(buffer);
  srcpf.rgbFromPixel(p, &r16, &g16, &b16);
  p = dstpf.pixelFromRGB(r16, g16, b16);
  memset(buffer, 0, sizeof(buffer));
  dstpf.bufferFromPixel(buffer, p);

  if (!verifyPixel(dstpf, srcpf, buffer))
    return false;

  makePixel(srcpf, buffer);

  p = srcpf.pixelFromBuffer(buffer);
  srcpf.rgbFromPixel(p, &r8, &g8, &b8);
  p = dstpf.pixelFromRGB(r8, g8, b8);
  memset(buffer, 0, sizeof(buffer));
  dstpf.bufferFromPixel(buffer, p);

  if (!verifyPixel(dstpf, srcpf, buffer))
    return false;

  return true;
}

struct TestEntry tests[] = {
  {"Pixel from pixel", testPixel},
  {"Buffer from buffer", testBuffer},
  {"Buffer to/from RGB", testRGB},
  {"Pixel to/from RGB", testPixelRGB},
};

static void doTests(const rfb::PixelFormat &dstpf,
                    const rfb::PixelFormat &srcpf)
{
  size_t i;
  char dstb[256], srcb[256];

  dstpf.print(dstb, sizeof(dstb));
  srcpf.print(srcb, sizeof(srcb));

  printf("\n");
  printf("%s to %s\n", srcb, dstb);
  printf("\n");

  for (i = 0;i < sizeof(tests)/sizeof(tests[0]);i++) {
    printf("    %s: ", tests[i].label);
    fflush(stdout);
    if (tests[i].fn(dstpf, srcpf))
      printf("OK");
    else
      printf("FAILED");
    printf("\n");
  }
}

int main(int argc, char **argv)
{
  rfb::PixelFormat dstpf, srcpf;

  printf("Pixel Conversion Correctness Test\n");

  /* rgb888 targets */

  dstpf.parse("rgb888");

  srcpf.parse("rgb888");
  doTests(dstpf, srcpf);

  srcpf.parse("bgr888");
  doTests(dstpf, srcpf);

  srcpf.parse("rgb565");
  doTests(dstpf, srcpf);

  srcpf.parse("rgb232");
  doTests(dstpf, srcpf);

  /* rgb565 targets */

  dstpf.parse("rgb565");

  srcpf.parse("rgb888");
  doTests(dstpf, srcpf);

  srcpf.parse("bgr565");
  doTests(dstpf, srcpf);

  srcpf.parse("rgb232");
  doTests(dstpf, srcpf);

  /* rgb232 targets */

  dstpf.parse("rgb232");

  srcpf.parse("rgb888");
  doTests(dstpf, srcpf);

  srcpf.parse("rgb565");
  doTests(dstpf, srcpf);

  srcpf.parse("bgr232");
  doTests(dstpf, srcpf);

  /* endian conversion (both ways) */

  dstpf = rfb::PixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16);
  srcpf = rfb::PixelFormat(32, 24, true, true, 255, 255, 255, 0, 8, 16);

  doTests(dstpf, srcpf);

  doTests(srcpf, dstpf);

  dstpf = rfb::PixelFormat(16, 16, false, true, 31, 63, 31, 0, 5, 11);
  srcpf = rfb::PixelFormat(16, 16, true, true, 31, 63, 31, 0, 5, 11);

  doTests(dstpf, srcpf);

  doTests(srcpf, dstpf);

  // Pesky case that is very asymetrical
  dstpf = rfb::PixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16);
  srcpf = rfb::PixelFormat(32, 24, true, true, 255, 255, 255, 0, 24, 8);

  doTests(dstpf, srcpf);

  doTests(srcpf, dstpf);
}
