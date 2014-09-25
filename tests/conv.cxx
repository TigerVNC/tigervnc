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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rfb/PixelFormat.h>

static const rdr::U8 pixelRed = 0xf1;
static const rdr::U8 pixelGreen = 0xc3;
static const rdr::U8 pixelBlue = 0x97;

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

  r <<= 8 - dstpf.redBits;
  g <<= 8 - dstpf.greenBits;
  b <<= 8 - dstpf.blueBits;

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
  int i, unaligned;
  rdr::U8 bufIn[1024 + 1], bufOut[1024 + 1];

  // Once aligned, and once unaligned
  for (unaligned = 0;unaligned < 2;unaligned++) {
    for (i = 0;i < 1024/4;i++)
      makePixel(srcpf, bufIn + unaligned + i*srcpf.bpp/8);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromBuffer(bufOut + unaligned, srcpf,
                           bufIn + unaligned, 1024/4);

    for (i = 0;i < 1024/4;i++) {
      if (!verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8))
        return false;
    }

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromBuffer(bufOut + unaligned, srcpf, bufIn + unaligned,
                           1024/4/10, 10, 1024/4/10, 1024/4/10);

    for (i = 0;i < 1024/4/10*10;i++) {
      if (!verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8))
        return false;
    }
  }

  return true;
}

static bool testRGB(const rfb::PixelFormat &dstpf,
                    const rfb::PixelFormat &srcpf)
{
  int i, unaligned;
  rdr::U8 bufIn[1024 + 1], bufRGB[1024 + 1], bufOut[1024 + 1];

  // Once aligned, and once unaligned
  for (unaligned = 0;unaligned < 2;unaligned++) {
    for (i = 0;i < 1024/4;i++)
      makePixel(srcpf, bufIn + unaligned + i*srcpf.bpp/8);

    memset(bufRGB, 0, sizeof(bufRGB));
    srcpf.rgbFromBuffer(bufRGB + unaligned, bufIn + unaligned, 1024/4);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromRGB(bufOut + unaligned, bufRGB + unaligned, 1024/4);

    for (i = 0;i < 1024/4;i++) {
      if (!verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8))
        return false;
    }

    memset(bufRGB, 0, sizeof(bufRGB));
    srcpf.rgbFromBuffer(bufRGB + unaligned, bufIn + unaligned,
                        1024/4/10, 1024/4/10, 10);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromRGB(bufOut + unaligned, bufRGB + unaligned,
                        1024/4/10, 1024/4/10, 10);

    for (i = 0;i < 1024/4/10*10;i++) {
      if (!verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8))
        return false;
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
  int i;
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
