/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2014-2023 Pierre Ossman for Cendio AB
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

#include <rdr/OutStream.h>
#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/PixelBuffer.h>
#include <rfb/JPEGEncoder.h>

using namespace rfb;

JPEGEncoder::JPEGEncoder(SConnection* conn_) :
  Encoder(conn_, encodingJPEG,
          (EncoderFlags)(EncoderUseNativePF | EncoderLossy), -1, 9)
{
}

JPEGEncoder::~JPEGEncoder()
{
}

bool JPEGEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingJPEG);
}

void JPEGEncoder::setQualityLevel(int level)
{
  jc.setQualityLevel(level);
}

void JPEGEncoder::setFineQualityLevel(int quality, int subsampling)
{
  jc.setFineQualityLevel(quality, subsampling);
}

int JPEGEncoder::getQualityLevel()
{
  return jc.getQualityLevel();
}

void JPEGEncoder::writeRect(const PixelBuffer* pb,
                            const Palette& /*palette*/)
{
  const uint8_t* buffer;
  int stride;

  const uint8_t* data;
  size_t len;

  rdr::OutStream* os;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  jc.clear();
  jc.compress(buffer, stride, pb->getRect(), pb->getPF());

  os = conn->getOutStream();

  data = jc.data();
  len = jc.length();

  // scan through the segments to look for the huffman table and the
  // quantization table
  while (true) {
    uint8_t type;
    size_t seglen;

    assert(len >= 2);

    assert(data[0] == 0xff);
    type = data[1];

    // segment without length?
    if ((type == 0x01) || ((type >= 0xd0) && (type <= 0xd9))) {
      os->writeBytes(data, 2);
      data += 2;
      len -= 2;
      continue;
    }

    assert(len >= 4);

    seglen = data[2] << 16 | data[3];

    assert(len >= (2 + seglen));

    if (type == 0xc4) {
      // huffman table

      size_t totallen;

      // there are multiple tables following each other
      totallen = 2 + seglen;
      while (true) {
        assert(len >= totallen + 1);
        assert(data[totallen] == 0xff);

        assert(len >= totallen + 2);
        if (data[totallen + 1] != 0xc4)
            break;

        seglen = data[totallen + 2] << 16 | data[totallen + 3];

        assert(len >= (totallen + 2 + seglen));

        totallen += 2 + seglen;
      }

      if ((totallen == lastHuffmanTables.size()) &&
          (memcmp(data, lastHuffmanTables.data(), totallen) == 0)) {
        // same table as last rect, so skip it
        data += totallen;
        len -= totallen;
        continue;
      }

      // store this for the next rect
      lastHuffmanTables.resize(totallen);
      memcpy(lastHuffmanTables.data(), data, totallen);

      os->writeBytes(data, totallen);
      data += totallen;
      len -= totallen;
      continue;
    }

    if (type == 0xdb) {
      // quantization table

      size_t totallen;

      // there are multiple tables following each other
      totallen = 2 + seglen;
      while (true) {
        assert(len >= totallen + 1);
        assert(data[totallen] == 0xff);

        assert(len >= totallen + 2);
        if (data[totallen + 1] != 0xdb)
            break;

        seglen = data[totallen + 2] << 16 | data[totallen + 3];

        assert(len >= (totallen + 2 + seglen));

        totallen += 2 + seglen;
      }

      if ((totallen == lastQuantTables.size()) &&
          (memcmp(data, lastQuantTables.data(), totallen) == 0)) {
        // same table as last rect, so skip it
        data += totallen;
        len -= totallen;
        continue;
      }

      // store this for the next rect
      lastQuantTables.resize(totallen);
      memcpy(lastQuantTables.data(), data, totallen);

      os->writeBytes(data, totallen);
      data += totallen;
      len -= totallen;
      continue;
    }

    if (type == 0xda) {
      // start of scan, i.e. the actual image data, so we are done
      os->writeBytes(data, len);
      break;
    }

    // some other segment
    os->writeBytes(data, 2 + seglen);
    data += 2 + seglen;
    len -= 2 + seglen;
  }
}

void JPEGEncoder::writeSolidRect(int width, int height,
                                 const PixelFormat& pf,
                                 const uint8_t* colour)
{
  // FIXME: Add a shortcut in the JPEG compressor to handle this case
  //        without having to use the default fallback which is very slow.
  Encoder::writeSolidRect(width, height, pf, colour);
}
