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

#include <rdr/OutStream.h>
#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/PixelBuffer.h>
#include <rfb/TightJPEGEncoder.h>
#include <rfb/TightConstants.h>

using namespace rfb;

TightJPEGEncoder::TightJPEGEncoder(SConnection* conn_) :
  Encoder(conn_, encodingTight,
          (EncoderFlags)(EncoderUseNativePF | EncoderLossy), -1, 9)
{
}

TightJPEGEncoder::~TightJPEGEncoder()
{
}

bool TightJPEGEncoder::isSupported()
{
  if (!conn->client.supportsEncoding(encodingTight))
    return false;

  // JPEG doesn't really care about the pixel format, but the
  // specification requires it to be at least 16bpp
  if (conn->client.pf().bpp < 16)
    return false;

  // Any one of these indicates support for JPEG
  if (conn->client.qualityLevel != -1)
    return true;
  if (conn->client.fineQualityLevel != -1)
    return true;
  if (conn->client.subsampling != -1)
    return true;

  // Tight support, but not JPEG
  return false;
}

void TightJPEGEncoder::setQualityLevel(int level)
{
  jc.setQualityLevel(level);
}

void TightJPEGEncoder::setFineQualityLevel(int quality, int subsampling)
{
  jc.setFineQualityLevel(quality, subsampling);
}

int TightJPEGEncoder::getQualityLevel()
{
  return jc.getQualityLevel();
}

void TightJPEGEncoder::writeRect(const PixelBuffer* pb,
                                 const Palette& /*palette*/)
{
  const uint8_t* buffer;
  int stride;

  rdr::OutStream* os;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  jc.clear();
  jc.compress(buffer, stride, pb->getRect(), pb->getPF());

  os = conn->getOutStream();

  os->writeU8(tightJpeg << 4);

  writeCompact(jc.length(), os);
  os->writeBytes(jc.data(), jc.length());
}

void TightJPEGEncoder::writeSolidRect(int width, int height,
                                      const PixelFormat& pf,
                                      const uint8_t* colour)
{
  // FIXME: Add a shortcut in the JPEG compressor to handle this case
  //        without having to use the default fallback which is very slow.
  Encoder::writeSolidRect(width, height, pf, colour);
}

void TightJPEGEncoder::writeCompact(uint32_t value, rdr::OutStream* os)
{
  // Copied from TightEncoder as it's overkill to inherit just for this
  uint8_t b;

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
