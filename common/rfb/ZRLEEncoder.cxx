/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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

using namespace rfb;

IntParameter zlibLevel("ZlibLevel","Zlib compression level",-1);

ZRLEEncoder::ZRLEEncoder(SConnection* conn)
  : Encoder(conn, encodingZRLE, EncoderPlain, 127),
  zos(0,zlibLevel), mos(129*1024)
{
  zos.setUnderlying(&mos);
}

ZRLEEncoder::~ZRLEEncoder()
{
  zos.setUnderlying(NULL);
}

bool ZRLEEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingZRLE);
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
        writeRawTile(tile, pb, palette);
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
                                 const rdr::U8* colour)
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
  const rdr::U8* buffer;
  int stride;

  buffer = pb->getBuffer(tile, &stride);

  switch (pb->getPF().bpp) {
  case 32:
    writePaletteTile(tile.width(), tile.height(),
                     (rdr::U32*)buffer, stride,
                     pb->getPF(), palette);
    break;
  case 16:
    writePaletteTile(tile.width(), tile.height(),
                     (rdr::U16*)buffer, stride,
                     pb->getPF(), palette);
    break;
  default:
    writePaletteTile(tile.width(), tile.height(),
                     (rdr::U8*)buffer, stride,
                     pb->getPF(), palette);
  }
}

void ZRLEEncoder::writePaletteRLETile(const Rect& tile, const PixelBuffer* pb,
                                      const Palette& palette)
{
  const rdr::U8* buffer;
  int stride;

  buffer = pb->getBuffer(tile, &stride);

  switch (pb->getPF().bpp) {
  case 32:
    writePaletteRLETile(tile.width(), tile.height(),
                        (rdr::U32*)buffer, stride,
                        pb->getPF(), palette);
    break;
  case 16:
    writePaletteRLETile(tile.width(), tile.height(),
                        (rdr::U16*)buffer, stride,
                        pb->getPF(), palette);
    break;
  default:
    writePaletteRLETile(tile.width(), tile.height(),
                        (rdr::U8*)buffer, stride,
                        pb->getPF(), palette);
  }
}

void ZRLEEncoder::writeRawTile(const Rect& tile, const PixelBuffer* pb,
                               const Palette& palette)
{
  const rdr::U8* buffer;
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
  rdr::U8 buffer[256*4];
  int i;

  if (pf.bpp == 32) {
    rdr::U32* buf;
    buf = (rdr::U32*)buffer;
    for (i = 0;i < palette.size();i++)
      *buf++ = palette.getColour(i);
  } else if (pf.bpp == 16) {
    rdr::U16* buf;
    buf = (rdr::U16*)buffer;
    for (i = 0;i < palette.size();i++)
      *buf++ = palette.getColour(i);
  } else {
    rdr::U8* buf;
    buf = (rdr::U8*)buffer;
    for (i = 0;i < palette.size();i++)
      *buf++ = palette.getColour(i);
  }

  writePixels(buffer, pf, palette.size());
}

void ZRLEEncoder::writePixels(const rdr::U8* buffer, const PixelFormat& pf,
                              unsigned int count)
{
  Pixel maxPixel;
  rdr::U8 pixBuf[4];

  maxPixel = pf.pixelFromRGB((rdr::U16)-1, (rdr::U16)-1, (rdr::U16)-1);
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

//
// Including BPP-dependent implementation of the encoder.
//

#define BPP 8
#include <rfb/ZRLEEncoderBPP.cxx>
#undef BPP
#define BPP 16
#include <rfb/ZRLEEncoderBPP.cxx>
#undef BPP
#define BPP 32
#include <rfb/ZRLEEncoderBPP.cxx>
#undef BPP
