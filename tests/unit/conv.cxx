/* Copyright 2013-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <string.h>

#include <list>

#include <gtest/gtest.h>

#include <rfb/PixelFormat.h>

static const uint8_t pixelRed = 0xf1;
static const uint8_t pixelGreen = 0xc3;
static const uint8_t pixelBlue = 0x97;

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
               uint8_t *buffer)
{
  rfb::Pixel p;

  p = 0;
  p |= (pixelRed >> (8 - pf.redBits)) << pf.redShift;
  p |= (pixelGreen >> (8 - pf.greenBits)) << pf.greenShift;
  p |= (pixelBlue >> (8 - pf.blueBits)) << pf.blueShift;

  // FIXME: Should we reimplement this as well?
  pf.bufferFromPixel(buffer, p);
}

void verifyPixel(const rfb::PixelFormat &dstpf,
                 const rfb::PixelFormat &srcpf,
                 const uint8_t *buffer)
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

  EXPECT_NEAR(r, pixelRed, re);
  EXPECT_NEAR(g, pixelGreen, ge);
  EXPECT_NEAR(b, pixelBlue, be);
}

}

typedef std::pair<rfb::PixelFormat, rfb::PixelFormat> TestPair;
typedef testing::TestWithParam<TestPair> Conv;

namespace rfb {

static std::ostream& operator<<(std::ostream& os, const PixelFormat& pf)
{
  char b[256];
  pf.print(b, sizeof(b));
  return os << b;
}

}

using rfb::makePixel;
using rfb::verifyPixel;

TEST_P(Conv, pixelFromPixel)
{
  rfb::Pixel p;
  uint8_t buffer[4];

  const rfb::PixelFormat &srcpf = GetParam().first;
  const rfb::PixelFormat &dstpf = GetParam().second;

  makePixel(srcpf, buffer);

  p = srcpf.pixelFromBuffer(buffer);
  p = dstpf.pixelFromPixel(srcpf, p);
  memset(buffer, 0, sizeof(buffer));
  dstpf.bufferFromPixel(buffer, p);

  verifyPixel(dstpf, srcpf, buffer);
}

TEST_P(Conv, bufferFromBuffer)
{
  int i, x, y, unaligned;
  uint8_t bufIn[fbMalloc], bufOut[fbMalloc];

  const rfb::PixelFormat &srcpf = GetParam().first;
  const rfb::PixelFormat &dstpf = GetParam().second;

  // Once aligned, and once unaligned
  for (unaligned = 0;unaligned < 2;unaligned++) {
    for (i = 0;i < fbArea;i++)
      makePixel(srcpf, bufIn + unaligned + i*srcpf.bpp/8);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromBuffer(bufOut + unaligned, srcpf,
                           bufIn + unaligned, fbArea);

    for (i = 0;i < fbArea;i++) {
      verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8);
      if (testing::Test::HasFailure())
        return;
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
        const uint8_t* pixel;
        pixel = bufOut + unaligned + (x + y*fbWidth)*dstpf.bpp/8;
        if (x < fbWidth/2) {
          verifyPixel(dstpf, srcpf, pixel);
        } else {
          const uint8_t zero[4] = { 0, 0, 0, 0 };
          EXPECT_EQ(memcmp(pixel, zero, dstpf.bpp/8), 0);
        }
        if (testing::Test::HasFailure())
          return;
      }
    }
  }
}

TEST_P(Conv, bufferToFromRGB)
{
  int i, x, y, unaligned;
  uint8_t bufIn[fbMalloc], bufRGB[fbMalloc], bufOut[fbMalloc];

  const rfb::PixelFormat &srcpf = GetParam().first;
  const rfb::PixelFormat &dstpf = GetParam().second;

  // Once aligned, and once unaligned
  for (unaligned = 0;unaligned < 2;unaligned++) {
    for (i = 0;i < fbArea;i++)
      makePixel(srcpf, bufIn + unaligned + i*srcpf.bpp/8);

    memset(bufRGB, 0, sizeof(bufRGB));
    srcpf.rgbFromBuffer(bufRGB + unaligned, bufIn + unaligned, fbArea);

    memset(bufOut, 0, sizeof(bufOut));
    dstpf.bufferFromRGB(bufOut + unaligned, bufRGB + unaligned, fbArea);

    for (i = 0;i < fbArea;i++) {
      verifyPixel(dstpf, srcpf, bufOut + unaligned + i*dstpf.bpp/8);
      if (testing::Test::HasFailure())
        return;
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
        const uint8_t* pixel;
        pixel = bufOut + unaligned + (x + y*fbWidth)*dstpf.bpp/8;
        if (x < fbWidth/2) {
          verifyPixel(dstpf, srcpf, pixel);
        } else {
          const uint8_t zero[4] = { 0, 0, 0, 0 };
          EXPECT_EQ(memcmp(pixel, zero, dstpf.bpp/8), 0);
        }
        if (testing::Test::HasFailure())
          return;
      }
    }
  }
}

TEST_P(Conv, pixelToFromRGB)
{
  rfb::Pixel p;
  uint16_t r16, g16, b16;
  uint8_t r8, g8, b8;
  uint8_t buffer[4];

  const rfb::PixelFormat &srcpf = GetParam().first;
  const rfb::PixelFormat &dstpf = GetParam().second;

  makePixel(srcpf, buffer);

  p = srcpf.pixelFromBuffer(buffer);
  srcpf.rgbFromPixel(p, &r16, &g16, &b16);
  p = dstpf.pixelFromRGB(r16, g16, b16);
  memset(buffer, 0, sizeof(buffer));
  dstpf.bufferFromPixel(buffer, p);

  verifyPixel(dstpf, srcpf, buffer);

  makePixel(srcpf, buffer);

  p = srcpf.pixelFromBuffer(buffer);
  srcpf.rgbFromPixel(p, &r8, &g8, &b8);
  p = dstpf.pixelFromRGB(r8, g8, b8);
  memset(buffer, 0, sizeof(buffer));
  dstpf.bufferFromPixel(buffer, p);

  verifyPixel(dstpf, srcpf, buffer);
}

static std::list<TestPair> paramGenerator()
{
  std::list<TestPair> params;
  rfb::PixelFormat dstpf, srcpf;

  /* rgb888 targets */

  dstpf.parse("rgb888");

  srcpf.parse("rgb888");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("bgr888");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("rgb565");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("rgb232");
  params.push_back(std::make_pair(srcpf, dstpf));

  /* rgb565 targets */

  dstpf.parse("rgb565");

  srcpf.parse("rgb888");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("bgr565");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("rgb232");
  params.push_back(std::make_pair(srcpf, dstpf));

  /* rgb232 targets */

  dstpf.parse("rgb232");

  srcpf.parse("rgb888");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("rgb565");
  params.push_back(std::make_pair(srcpf, dstpf));

  srcpf.parse("bgr232");
  params.push_back(std::make_pair(srcpf, dstpf));

  /* endian conversion (both ways) */

  dstpf = rfb::PixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16);
  srcpf = rfb::PixelFormat(32, 24, true, true, 255, 255, 255, 0, 8, 16);

  params.push_back(std::make_pair(srcpf, dstpf));
  params.push_back(std::make_pair(dstpf, srcpf));

  dstpf = rfb::PixelFormat(16, 16, false, true, 31, 63, 31, 0, 5, 11);
  srcpf = rfb::PixelFormat(16, 16, true, true, 31, 63, 31, 0, 5, 11);

  params.push_back(std::make_pair(srcpf, dstpf));
  params.push_back(std::make_pair(dstpf, srcpf));

  // Pesky case that is very asymetrical
  dstpf = rfb::PixelFormat(32, 24, false, true, 255, 255, 255, 0, 8, 16);
  srcpf = rfb::PixelFormat(32, 24, true, true, 255, 255, 255, 0, 24, 8);

  params.push_back(std::make_pair(srcpf, dstpf));
  params.push_back(std::make_pair(dstpf, srcpf));

  return params;
}

INSTANTIATE_TEST_SUITE_P(, Conv, testing::ValuesIn(paramGenerator()));

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
