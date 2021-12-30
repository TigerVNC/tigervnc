/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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

#include <assert.h>

#include <rdr/OutStream.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Palette.h>
#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/TightEncoder.h>
#include <rfb/TightConstants.h>

using namespace rfb;

struct TightConf {
  int idxZlibLevel, monoZlibLevel, rawZlibLevel;
};

//
// Compression level stuff. The following array contains zlib
// settings for each of 10 compression levels (0..9).
//
// NOTE: The parameters used in this encoder are the result of painstaking
// research by The VirtualGL Project using RFB session captures from a variety
// of both 2D and 3D applications.  See http://www.VirtualGL.org for the full
// reports.

static const TightConf conf[10] = {
  { 0, 0, 0 }, // 0
  { 1, 1, 1 }, // 1
  { 3, 3, 2 }, // 2
  { 5, 5, 2 }, // 3
  { 6, 7, 3 }, // 4
  { 7, 8, 4 }, // 5
  { 7, 8, 5 }, // 6
  { 8, 9, 6 }, // 7
  { 9, 9, 7 }, // 8
  { 9, 9, 9 }  // 9
};

TightEncoder::TightEncoder(SConnection* conn) :
  Encoder(conn, encodingTight, EncoderPlain, 256)
{
  setCompressLevel(-1);
}

TightEncoder::~TightEncoder()
{
}

bool TightEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingTight);
}

void TightEncoder::setCompressLevel(int level)
{
  if (level < 0 || level > 9)
    level = 2;

  idxZlibLevel = conf[level].idxZlibLevel;
  monoZlibLevel = conf[level].monoZlibLevel;
  rawZlibLevel = conf[level].rawZlibLevel;
}

void TightEncoder::writeRect(const PixelBuffer* pb, const Palette& palette)
{
  switch (palette.size()) {
  case 0:
    writeFullColourRect(pb, palette);
    break;
  case 1:
    Encoder::writeSolidRect(pb, palette);
    break;
  case 2:
    writeMonoRect(pb, palette);
    break;
  default:
    writeIndexedRect(pb, palette);
  }
}

void TightEncoder::writeSolidRect(int width, int height,
                                  const PixelFormat& pf,
                                  const rdr::U8* colour)
{
  rdr::OutStream* os;

  os = conn->getOutStream();

  os->writeU8(tightFill << 4);
  writePixels(colour, pf, 1, os);
}

void TightEncoder::writeMonoRect(const PixelBuffer* pb, const Palette& palette)
{
  const rdr::U8* buffer;
  int stride;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  switch (pb->getPF().bpp) {
  case 32:
    writeMonoRect(pb->width(), pb->height(), (rdr::U32*)buffer, stride,
                  pb->getPF(), palette);
    break;
  case 16:
    writeMonoRect(pb->width(), pb->height(), (rdr::U16*)buffer, stride,
                  pb->getPF(), palette);
    break;
  default:
    writeMonoRect(pb->width(), pb->height(), (rdr::U8*)buffer, stride,
                  pb->getPF(), palette);
  }
}

void TightEncoder::writeIndexedRect(const PixelBuffer* pb, const Palette& palette)
{
  const rdr::U8* buffer;
  int stride;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  switch (pb->getPF().bpp) {
  case 32:
    writeIndexedRect(pb->width(), pb->height(), (rdr::U32*)buffer, stride,
                     pb->getPF(), palette);
    break;
  case 16:
    writeIndexedRect(pb->width(), pb->height(), (rdr::U16*)buffer, stride,
                     pb->getPF(), palette);
    break;
  default:
    // It's more efficient to just do raw pixels
    writeFullColourRect(pb, palette);
  }
}

void TightEncoder::writeFullColourRect(const PixelBuffer* pb, const Palette& palette)
{
  const int streamId = 0;

  rdr::OutStream* os;
  rdr::OutStream* zos;
  int length;

  const rdr::U8* buffer;
  int stride, h;

  os = conn->getOutStream();

  os->writeU8(streamId << 4);

  // Set up compression
  if ((pb->getPF().bpp != 32) || !pb->getPF().is888())
    length = pb->getRect().area() * pb->getPF().bpp/8;
  else
    length = pb->getRect().area() * 3;

  zos = getZlibOutStream(streamId, rawZlibLevel, length);

  // And then just dump all the raw pixels
  buffer = pb->getBuffer(pb->getRect(), &stride);
  h = pb->height();

  while (h--) {
    writePixels(buffer, pb->getPF(), pb->width(), zos);
    buffer += stride * pb->getPF().bpp/8;
  }

  // Finish the zlib stream
  flushZlibOutStream(zos);
}

void TightEncoder::writePixels(const rdr::U8* buffer, const PixelFormat& pf,
                               unsigned int count, rdr::OutStream* os)
{
  rdr::U8 rgb[2048];

  if ((pf.bpp != 32) || !pf.is888()) {
    os->writeBytes(buffer, count * pf.bpp/8);
    return;
  }

  while (count) {
    unsigned int iter_count;

    iter_count = sizeof(rgb)/3;
    if (iter_count > count)
      iter_count = count;

    pf.rgbFromBuffer(rgb, buffer, iter_count);
    os->writeBytes(rgb, iter_count * 3);

    buffer += iter_count * pf.bpp/8;
    count -= iter_count;
  }
}

void TightEncoder::writeCompact(rdr::OutStream* os, rdr::U32 value)
{
  rdr::U8 b;
  b = value & 0x7F;
  if (value <= 0x7F) {
    os->writeU8(b);
  } else {
    os->writeU8(b | 0x80);
    b = value >> 7 & 0x7F;
    if (value <= 0x3FFF) {
      os->writeU8(b);
    } else {
      os->writeU8(b | 0x80);
      os->writeU8(value >> 14 & 0xFF);
    }
  }
}

rdr::OutStream* TightEncoder::getZlibOutStream(int streamId, int level, size_t length)
{
  // Minimum amount of data to be compressed. This value should not be
  // changed, doing so will break compatibility with existing clients.
  if (length < 12)
    return conn->getOutStream();

  assert(streamId >= 0);
  assert(streamId < 4);

  zlibStreams[streamId].setUnderlying(&memStream);
  zlibStreams[streamId].setCompressionLevel(level);
  zlibStreams[streamId].cork(true);

  return &zlibStreams[streamId];
}

void TightEncoder::flushZlibOutStream(rdr::OutStream* os_)
{
  rdr::OutStream* os;
  rdr::ZlibOutStream* zos;

  zos = dynamic_cast<rdr::ZlibOutStream*>(os_);
  if (zos == NULL)
    return;

  zos->cork(false);
  zos->flush();
  zos->setUnderlying(NULL);

  os = conn->getOutStream();

  writeCompact(os, memStream.length());
  os->writeBytes(memStream.data(), memStream.length());
  memStream.clear();
}

//
// Including BPP-dependent implementation of the encoder.
//

#define BPP 8
#include <rfb/TightEncoderBPP.cxx>
#undef BPP
#define BPP 16
#include <rfb/TightEncoderBPP.cxx>
#undef BPP
#define BPP 32
#include <rfb/TightEncoderBPP.cxx>
#undef BPP
