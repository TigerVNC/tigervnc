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
#include <rfb/Exception.h>
#include <rfb/encodings.h>
#include <rfb/Palette.h>
#include <rfb/SConnection.h>
#include <rfb/ZRLEEncoder.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("ZRLEEncoder");

IntParameter zlibLevel("ZlibLevel","[DEPRECATED] Zlib compression level",-1);

ZRLEEncoder::ZRLEEncoder(SConnection* conn_)
  : Encoder(conn_, encodingZRLE, EncoderPlain, 127),
  zos(nullptr, 2), mos(129*1024)
{
  if (zlibLevel != -1) {
    vlog.info("Warning: The ZlibLevel option is deprecated and is "
              "ignored by the server. The compression level can be set "
              "by the client instead.");
  }
  zos.setUnderlying(&mos);
}

ZRLEEncoder::~ZRLEEncoder()
{
  zos.setUnderlying(nullptr);
}

bool ZRLEEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingZRLE);
}

void ZRLEEncoder::setCompressLevel(int level)
{
  zos.setCompressionLevel(level);
}

void ZRLEEncoder::writeRect(const PixelBuffer* pb, const Palette& palette)
{
  int x, y;
  Rect tile;

  rdr::OutStream* os;

  // A bit of a special case
  if (palette.size() == 1) {
    Encoder::writeSolidRect(pb, palette);
    return;
  }

  for (y = 0;y < pb->height();y += 64) {
    tile.tl.y = y;
    tile.br.y = y + 64;
    if (tile.br.y > pb->height())
      tile.br.y = pb->height();

    for (x = 0;x < pb->width();x += 64) {
      tile.tl.x = x;
      tile.br.x = x + 64;
      if (tile.br.x > pb->width())
        tile.br.x = pb->width();

      if (palette.size() == 0)
        writeRawTile(tile, pb);
      else if (palette.size() <= 16)
        writePaletteTile(tile, pb, palette);
      else
        writePaletteRLETile(tile, pb, palette);
    }
  }

  zos.flush();

  os = conn->getOutStream();

  os->writeU32(mos.length());
  os->writeBytes(mos.data(), mos.length());

  mos.clear();
}

void ZRLEEncoder::writeSolidRect(int width, int height,
                                 const PixelFormat& pf,
                                 const uint8_t* colour)
{
  int tiles;

  rdr::OutStream* os;

  tiles = ((width + 63)/64) * ((height + 63)/64);

  while (tiles--) {
    zos.writeU8(1);
    writePixels(colour, pf, 1);
  }

  zos.flush();

  os = conn->getOutStream();

  os->writeU32(mos.length());
  os->writeBytes(mos.data(), mos.length());

  mos.clear();
}

void ZRLEEncoder::writePaletteTile(const Rect& tile, const PixelBuffer* pb,
                                   const Palette& palette)
{
  const uint8_t* buffer;
  int stride;

  buffer = pb->getBuffer(tile, &stride);

  switch (pb->getPF().bpp) {
  case 32:
    writePaletteTile(tile.width(), tile.height(),
                     (uint32_t*)buffer, stride,
                     pb->getPF(), palette);
    break;
  case 16:
    writePaletteTile(tile.width(), tile.height(),
                     (uint16_t*)buffer, stride,
                     pb->getPF(), palette);
    break;
  default:
    writePaletteTile(tile.width(), tile.height(),
                     (uint8_t*)buffer, stride,
                     pb->getPF(), palette);
  }
}

void ZRLEEncoder::writePaletteRLETile(const Rect& tile, const PixelBuffer* pb,
                                      const Palette& palette)
{
  const uint8_t* buffer;
  int stride;

  buffer = pb->getBuffer(tile, &stride);

  switch (pb->getPF().bpp) {
  case 32:
    writePaletteRLETile(tile.width(), tile.height(),
                        (uint32_t*)buffer, stride,
                        pb->getPF(), palette);
    break;
  case 16:
    writePaletteRLETile(tile.width(), tile.height(),
                        (uint16_t*)buffer, stride,
                        pb->getPF(), palette);
    break;
  default:
    writePaletteRLETile(tile.width(), tile.height(),
                        (uint8_t*)buffer, stride,
                        pb->getPF(), palette);
  }
}

void ZRLEEncoder::writeRawTile(const Rect& tile, const PixelBuffer* pb)
{
  const uint8_t* buffer;
  int stride;

  int w, h, stride_bytes;

  buffer = pb->getBuffer(tile, &stride);

  zos.writeU8(0); // Empty palette (i.e. raw pixels)

  w = tile.width();
  h = tile.height();
  stride_bytes = stride * pb->getPF().bpp/8;
  while (h--) {
    writePixels(buffer, pb->getPF(), w);
    buffer += stride_bytes;
  }
}

void ZRLEEncoder::writePalette(const PixelFormat& pf, const Palette& palette)
{
  uint8_t buffer[256*4];
  int i;

  if (pf.bpp == 32) {
    uint32_t* buf;
    buf = (uint32_t*)buffer;
    for (i = 0;i < palette.size();i++)
      *buf++ = palette.getColour(i);
  } else if (pf.bpp == 16) {
    uint16_t* buf;
    buf = (uint16_t*)buffer;
    for (i = 0;i < palette.size();i++)
      *buf++ = palette.getColour(i);
  } else {
    uint8_t* buf;
    buf = (uint8_t*)buffer;
    for (i = 0;i < palette.size();i++)
      *buf++ = palette.getColour(i);
  }

  writePixels(buffer, pf, palette.size());
}

void ZRLEEncoder::writePixels(const uint8_t* buffer, const PixelFormat& pf,
                              unsigned int count)
{
  Pixel maxPixel;
  uint8_t pixBuf[4];

  maxPixel = pf.pixelFromRGB((uint16_t)-1, (uint16_t)-1, (uint16_t)-1);
  pf.bufferFromPixel(pixBuf, maxPixel);

  if ((pf.bpp != 32) || ((pixBuf[0] != 0) && (pixBuf[3] != 0))) {
    zos.writeBytes(buffer, count * (pf.bpp/8));
    return;
  }

  if (pixBuf[0] == 0)
    buffer++;

  while (count--) {
    zos.writeBytes(buffer, 3);
    buffer += 4;
  }
}

template<class T>
void ZRLEEncoder::writePaletteTile(int width, int height,
                                   const T* buffer, int stride,
                                   const PixelFormat& pf,
                                   const Palette& palette)
{
  const int bitsPerPackedPixel[] = {
    0, 1, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
  };

  int bppp;
  int pad;

  assert(palette.size() > 1);
  assert(palette.size() <= 16);

  zos.writeU8(palette.size());
  writePalette(pf, palette);

  bppp = bitsPerPackedPixel[palette.size()-1];
  pad = stride - width;

  for (int i = 0; i < height; i++) {
    int w;

    uint8_t nbits = 0;
    uint8_t byte = 0;

    w = width;
    while (w--) {
      T pix = *buffer++;
      uint8_t index = palette.lookup(pix);
      byte = (byte << bppp) | index;
      nbits += bppp;
      if (nbits >= 8) {
        zos.writeU8(byte);
        nbits = 0;
      }
    }
    if (nbits > 0) {
      byte <<= 8 - nbits;
      zos.writeU8(byte);
    }

    buffer += pad;
  }
}

template<class T>
void ZRLEEncoder::writePaletteRLETile(int width, int height,
                                      const T* buffer, int stride,
                                      const PixelFormat& pf,
                                      const Palette& palette)
{
  int pad;

  T prevColour;
  int runLength;

  assert(palette.size() > 1);
  assert(palette.size() <= 127);

  zos.writeU8(palette.size() | 0x80);
  writePalette(pf, palette);

  pad = stride - width;

  prevColour = *buffer;
  runLength = 0;

  while (height--) {
    int w = width;
    while (w--) {
      if (prevColour != *buffer) {
        if (runLength == 1)
          zos.writeU8(palette.lookup(prevColour));
        else {
          zos.writeU8(palette.lookup(prevColour) | 0x80);

          while (runLength > 255) {
            zos.writeU8(255);
            runLength -= 255;
          }
          zos.writeU8(runLength - 1);
        }

        prevColour = *buffer;
        runLength = 0;
      }

      runLength++;
      buffer++;
    }
    buffer += pad;
  }
  if (runLength == 1)
    zos.writeU8(palette.lookup(prevColour));
  else {
    zos.writeU8(palette.lookup(prevColour) | 0x80);

    while (runLength > 255) {
      zos.writeU8(255);
      runLength -= 255;
    }
    zos.writeU8(runLength - 1);
  }
}
