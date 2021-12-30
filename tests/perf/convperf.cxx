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
#include <time.h>

#include <rfb/PixelFormat.h>

#include "util.h"

static const int tile = 64;
static const int fbsize = 4096;

static rdr::U8 *fb1, *fb2;

typedef void (*testfn) (rfb::PixelFormat&, rfb::PixelFormat&, rdr::U8*, rdr::U8*);

struct TestEntry {
  const char *label;
  testfn fn;
};

static void testMemcpy(rfb::PixelFormat &dstpf, rfb::PixelFormat &srcpf,
                       rdr::U8 *dst, rdr::U8 *src)
{
  int h;
  h = tile;
  while (h--) {
    memcpy(dst, src, tile * dstpf.bpp/8);
    dst += fbsize * dstpf.bpp/8;
    src += fbsize * dstpf.bpp/8;
  }
}

static void testBuffer(rfb::PixelFormat &dstpf, rfb::PixelFormat &srcpf,
                       rdr::U8 *dst, rdr::U8 *src)
{
  dstpf.bufferFromBuffer(dst, srcpf, src, tile, tile, fbsize, fbsize);
}

static void testToRGB(rfb::PixelFormat &dstpf, rfb::PixelFormat &srcpf,
                      rdr::U8 *dst, rdr::U8 *src)
{
  srcpf.rgbFromBuffer(dst, src, tile, fbsize, tile);
}

static void testFromRGB(rfb::PixelFormat &dstpf, rfb::PixelFormat &srcpf,
                        rdr::U8 *dst, rdr::U8 *src)
{
  dstpf.bufferFromRGB(dst, src, tile, fbsize, tile);
}

static void doTest(testfn fn, rfb::PixelFormat &dstpf, rfb::PixelFormat &srcpf)
{
  startCpuCounter();

  for (int i = 0;i < 10000;i++) {
    int x, y;
    rdr::U8 *dst, *src;
    x = rand() % (fbsize - tile);
    y = rand() % (fbsize - tile);
    dst = fb1 + (x + y * fbsize) * dstpf.bpp/8;
    src = fb2 + (x + y * fbsize) * srcpf.bpp/8;
    fn(dstpf, srcpf, dst, src);
  }

  endCpuCounter();

  float data, time;

  data = (double)tile * tile * 10000;
  time = getCpuCounter();

  printf("%g", data / (1000.0*1000.0) / time);
}

struct TestEntry tests[] = {
  {"memcpy", testMemcpy},
  {"bufferFromBuffer", testBuffer},
  {"rgbFromBuffer", testToRGB},
  {"bufferFromRGB", testFromRGB},
};

static void doTests(rfb::PixelFormat &dstpf, rfb::PixelFormat &srcpf)
{
  size_t i;
  char dstb[256], srcb[256];

  dstpf.print(dstb, sizeof(dstb));
  srcpf.print(srcb, sizeof(srcb));

  printf("%s,%s", srcb, dstb);

  for (i = 0;i < sizeof(tests)/sizeof(tests[0]);i++) {
    printf(",");
    doTest(tests[i].fn, dstpf, srcpf);
  }

  printf("\n");
}

int main(int argc, char **argv)
{
  size_t bufsize;

  time_t t;
  char datebuffer[256];

  size_t i;

  bufsize = fbsize * fbsize * 4;

  fb1 = new rdr::U8[bufsize];
  fb2 = new rdr::U8[bufsize];

  for (i = 0;i < bufsize;i++) {
    fb1[i] = rand();
    fb2[i] = rand();
  }

  time(&t);
  strftime(datebuffer, sizeof(datebuffer), "%Y-%m-%d %H:%M UTC", gmtime(&t));

  printf("# Pixel Conversion Performance Test %s\n", datebuffer);
  printf("#\n");
  printf("# Frame buffer: %dx%d pixels\n", fbsize, fbsize);
  printf("# Tile size: %dx%d pixels\n", tile, tile);
  printf("#\n");
  printf("# Note: Results are Mpixels/sec\n");
  printf("#\n");

  printf("Source format,Destination Format");
  for (i = 0;i < sizeof(tests)/sizeof(tests[0]);i++)
    printf(",%s", tests[i].label);
  printf("\n");

  rfb::PixelFormat dstpf, srcpf;

  /* rgb888 targets */

  printf("\n");

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

  printf("\n");

  dstpf.parse("rgb565");

  srcpf.parse("rgb888");
  doTests(dstpf, srcpf);

  srcpf.parse("bgr565");
  doTests(dstpf, srcpf);

  srcpf.parse("rgb232");
  doTests(dstpf, srcpf);

  /* rgb232 targets */

  printf("\n");

  dstpf.parse("rgb232");

  srcpf.parse("rgb888");
  doTests(dstpf, srcpf);

  srcpf.parse("rgb565");
  doTests(dstpf, srcpf);

  srcpf.parse("bgr232");
  doTests(dstpf, srcpf);

  /* rgb565 with endian conversion (both ways) */

  printf("\n");

  dstpf = rfb::PixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16);
  srcpf = rfb::PixelFormat(32, 24, true, true, 255, 255, 255, 0, 8, 16);

  doTests(srcpf, dstpf);

  doTests(dstpf, srcpf);

  dstpf = rfb::PixelFormat(16, 16, false, true, 31, 63, 31, 0, 5, 11);
  srcpf = rfb::PixelFormat(16, 16, true, true, 31, 63, 31, 0, 5, 11);

  doTests(srcpf, dstpf);

  doTests(dstpf, srcpf);

  return 0;
}

