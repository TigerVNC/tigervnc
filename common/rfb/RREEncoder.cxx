/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <rdr/OutStream.h>
#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/PixelFormat.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Palette.h>
#include <rfb/RREEncoder.h>

using namespace rfb;

RREEncoder::RREEncoder(SConnection* conn_) :
  Encoder(conn_, encodingRRE, EncoderPlain)
{
}

RREEncoder::~RREEncoder()
{
}

bool RREEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingRRE);
}

void RREEncoder::writeRect(const PixelBuffer* pb, const Palette& palette)
{
  uint8_t* imageBuf;
  int stride;
  uint32_t bg;

  int w = pb->width();
  int h = pb->height();

  if (palette.size() == 1) {
    Encoder::writeSolidRect(pb, palette);
    return;
  }

  // We have to have our own copy of the data as we modify it as
  // we find subrects.
  bufferCopy.setPF(pb->getPF());
  bufferCopy.setSize(w, h);

  imageBuf = bufferCopy.getBufferRW(pb->getRect(), &stride);
  pb->getImage(imageBuf, pb->getRect());

  if (palette.size() > 0)
    bg = palette.getColour(0);
  else {
    // Some crazy person is using this encoder for high colour
    // data. Just pick the first pixel as the background colour.
    bg = 0;
    memcpy(&bg, imageBuf, pb->getPF().bpp/8);
  }

  int nSubrects = -1;
  switch (pb->getPF().bpp) {
  case 8:
    nSubrects = rreEncode<uint8_t>((uint8_t*)imageBuf, w, h, &mos, bg);
    break;
  case 16:
    nSubrects = rreEncode<uint16_t>((uint16_t*)imageBuf, w, h, &mos, bg);
    break;
  case 32:
    nSubrects = rreEncode<uint32_t>((uint32_t*)imageBuf, w, h, &mos, bg);
    break;
  }

  bufferCopy.commitBufferRW(pb->getRect());

  rdr::OutStream* os = conn->getOutStream();
  os->writeU32(nSubrects);
  os->writeBytes(mos.data(), mos.length());
  mos.clear();
}

void RREEncoder::writeSolidRect(int /*width*/, int /*height*/,
                                const PixelFormat& pf,
                                const uint8_t* colour)
{
  rdr::OutStream* os;

  os = conn->getOutStream();

  os->writeU32(0);
  os->writeBytes(colour, pf.bpp/8);
}

template<class T>
inline void RREEncoder::writePixel(rdr::OutStream* os, T pixel)
{
  if (sizeof(T) == 1)
    os->writeOpaque8(pixel);
  else if (sizeof(T) == 2)
    os->writeOpaque16(pixel);
  else if (sizeof(T) == 4)
    os->writeOpaque32(pixel);
}

template<class T>
int RREEncoder::rreEncode(T* data, int w, int h,
                          rdr::OutStream* os, T bg)
{
  writePixel(os, bg);

  int nSubrects = 0;

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
          if (*ptr++ != *data) goto endOfHorizSubrect;
        ptr += w - sw;
        sh++;
      }
    endOfHorizSubrect:

      // Find vertical subrect
      int vh;
      for (vh = sh; vh < h-y; vh++)
        if (data[vh*w] != *data) break;

      if (vh != sh) {
        ptr = data+1;
        int vw;
        for (vw = 1; vw < sw; vw++) {
          for (int i = 0; i < vh; i++)
            if (ptr[i*w] != *data) goto endOfVertSubrect;
          ptr++;
        }
      endOfVertSubrect:

        // If vertical subrect bigger than horizontal then use that.
        if (sw*sh < vw*vh) {
          sw = vw;
          sh = vh;
        }
      }

      nSubrects++;
      writePixel(os, *data);
      os->writeU16(x);
      os->writeU16(y);
      os->writeU16(sw);
      os->writeU16(sh);

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

  return nSubrects;
}
