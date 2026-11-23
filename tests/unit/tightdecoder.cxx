/* This is free software; you can redistribute it and/or modify
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

#include <algorithm>
#include <array>
#include <iterator>

#include <gtest/gtest.h>

#include <rdr/MemInStream.h>
#include <rdr/MemOutStream.h>
#include <rfb/Exception.h>
#include <rfb/PixelBuffer.h>
#include <rfb/ServerParams.h>
#include <rfb/TightDecoder.h>

namespace {
  class FragmentedInStream : public rdr::MemInStream {
  public:
    explicit FragmentedInStream(const uint8_t* data)
      : MemInStream(data, 0)
    {
    }

    void expose(size_t bytes)
    {
      end += bytes;
    }

  private:
    bool overrun(size_t) override
    {
      return false;
    }
  };

  class TightDecoderTest : public testing::Test {
  protected:
    TightDecoderTest()
      : pf(32, 24, false, true, 255, 255, 255, 16, 8, 0)
    {
      server.setPF(pf);
    }

    std::array<uint8_t, 3> readPixel(const rfb::PixelBuffer& pb, int x, int y)
    {
      int stride;
      const uint8_t* pixel = pb.getBuffer(core::Rect(x, y, x + 1, y + 1),
                                          &stride);
      std::array<uint8_t, 3> rgb;
      pf.rgbFromBuffer(rgb.data(), pixel, 1);
      return rgb;
    }

    rfb::PixelFormat pf;
    rfb::ServerParams server;
    rfb::TightDecoder decoder;
    rdr::MemOutStream out;
  };
}

TEST_F(TightDecoderTest, UncompressedCopyWithAndWithoutLength)
{
  const uint8_t wire[] = {
    0xa0, 12, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120,
    0xa0, 130, 140, 150,
  };
  rdr::MemInStream in(wire, sizeof(wire));
  rfb::ManagedPixelBuffer pb(pf, 5, 1);
  const core::Rect copy(0, 0, 4, 1);
  const core::Rect smallCopy(4, 0, 5, 1);

  ASSERT_TRUE(decoder.readRect(copy, &in, server, &out));
  decoder.decodeRect(copy, out.data(), out.length(), server, &pb);
  out.clear();
  ASSERT_TRUE(decoder.readRect(smallCopy, &in, server, &out));
  decoder.decodeRect(smallCopy, out.data(), out.length(), server, &pb);

  EXPECT_EQ(in.pos(), sizeof(wire));
  EXPECT_EQ(readPixel(pb, 0, 0), (std::array<uint8_t, 3>{10, 20, 30}));
  EXPECT_EQ(readPixel(pb, 3, 0), (std::array<uint8_t, 3>{100, 110, 120}));
  EXPECT_EQ(readPixel(pb, 4, 0), (std::array<uint8_t, 3>{130, 140, 150}));
}

TEST_F(TightDecoderTest, FragmentedUncompressedPalette)
{
  // 9 pixels per row, 2 bytes of indices per row, compact length 128.
  const uint8_t header[] = {
    0xe1, 1, 1, 10, 20, 30, 40, 50, 60, 0x80, 0x01,
  };
  const size_t payloadSize = 128;
  const size_t firstPayloadChunk = 5;
  std::array<uint8_t, sizeof(header) + payloadSize> wire = {};
  std::copy(std::begin(header), std::end(header), wire.begin());
  wire[sizeof(header)] = 0x80;
  wire[wire.size() - 1] = 0x80;

  FragmentedInStream in(wire.data());
  const core::Rect rect(0, 0, 9, 64);
  rfb::ManagedPixelBuffer pb(pf, 9, 64);

  in.expose(sizeof(header) - 1);
  ASSERT_FALSE(decoder.readRect(rect, &in, server, &out));
  in.expose(1 + firstPayloadChunk);
  ASSERT_FALSE(decoder.readRect(rect, &in, server, &out));
  in.expose(payloadSize - firstPayloadChunk);
  ASSERT_TRUE(decoder.readRect(rect, &in, server, &out));
  decoder.decodeRect(rect, out.data(), out.length(), server, &pb);

  EXPECT_EQ(in.pos(), wire.size());
  EXPECT_EQ(readPixel(pb, 0, 0), (std::array<uint8_t, 3>{40, 50, 60}));
  EXPECT_EQ(readPixel(pb, 1, 0), (std::array<uint8_t, 3>{10, 20, 30}));
  EXPECT_EQ(readPixel(pb, 8, 63), (std::array<uint8_t, 3>{40, 50, 60}));
}

TEST_F(TightDecoderTest, RejectsIncorrectUncompressedLength)
{
  const uint8_t wire[] = {0xa0, 11, 0, 0};
  rdr::MemInStream in(wire, sizeof(wire));
  const core::Rect rect(0, 0, 4, 1);

  EXPECT_THROW(decoder.readRect(rect, &in, server, &out), rfb::protocol_error);
}
