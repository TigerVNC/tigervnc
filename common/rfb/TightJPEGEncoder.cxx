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

struct TightJPEGConfiguration {
    int quality;
    int subsampling;
};

// NOTE:  The JPEG quality and subsampling levels below were obtained
// experimentally by the VirtualGL Project.  They represent the approximate
// average compression ratios listed below, as measured across the set of
// every 10th frame in the SPECviewperf 9 benchmark suite.
//
// 9 = JPEG quality 100, no subsampling (ratio ~= 10:1)
//     [this should be lossless, except for round-off error]
// 8 = JPEG quality 92,  no subsampling (ratio ~= 20:1)
//     [this should be perceptually lossless, based on current research]
// 7 = JPEG quality 86,  no subsampling (ratio ~= 25:1)
// 6 = JPEG quality 79,  no subsampling (ratio ~= 30:1)
// 5 = JPEG quality 77,  4:2:2 subsampling (ratio ~= 40:1)
// 4 = JPEG quality 62,  4:2:2 subsampling (ratio ~= 50:1)
// 3 = JPEG quality 42,  4:2:2 subsampling (ratio ~= 60:1)
// 2 = JPEG quality 41,  4:2:0 subsampling (ratio ~= 70:1)
// 1 = JPEG quality 29,  4:2:0 subsampling (ratio ~= 80:1)
// 0 = JPEG quality 15,  4:2:0 subsampling (ratio ~= 100:1)

static const struct TightJPEGConfiguration conf[10] = {
  {  15, subsample4X }, // 0
  {  29, subsample4X }, // 1
  {  41, subsample4X }, // 2
  {  42, subsample2X }, // 3
  {  62, subsample2X }, // 4
  {  77, subsample2X }, // 5
  {  79, subsampleNone }, // 6
  {  86, subsampleNone }, // 7
  {  92, subsampleNone }, // 8
  { 100, subsampleNone }  // 9
};


TightJPEGEncoder::TightJPEGEncoder(SConnection* conn) :
  Encoder(conn, encodingTight,
          (EncoderFlags)(EncoderUseNativePF | EncoderLossy), -1, 9),
  qualityLevel(-1), fineQuality(-1), fineSubsampling(subsampleUndefined)
{
}

TightJPEGEncoder::~TightJPEGEncoder()
{
}

bool TightJPEGEncoder::isSupported()
{
  if (!conn->client.supportsEncoding(encodingTight))
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
  qualityLevel = level;
}

void TightJPEGEncoder::setFineQualityLevel(int quality, int subsampling)
{
  fineQuality = quality;
  fineSubsampling = subsampling;
}

int TightJPEGEncoder::getQualityLevel()
{
  return qualityLevel;
}

void TightJPEGEncoder::writeRect(const PixelBuffer* pb, const Palette& palette)
{
  const rdr::U8* buffer;
  int stride;

  int quality, subsampling;

  rdr::OutStream* os;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  if (qualityLevel >= 0 && qualityLevel <= 9) {
    quality = conf[qualityLevel].quality;
    subsampling = conf[qualityLevel].subsampling;
  } else {
    quality = -1;
    subsampling = subsampleUndefined;
  }

  // Fine settings trump level
  if (fineQuality != -1)
    quality = fineQuality;
  if (fineSubsampling != subsampleUndefined)
    subsampling = fineSubsampling;

  jc.clear();
  jc.compress(buffer, stride, pb->getRect(),
              pb->getPF(), quality, subsampling);

  os = conn->getOutStream();

  os->writeU8(tightJpeg << 4);

  writeCompact(jc.length(), os);
  os->writeBytes(jc.data(), jc.length());
}

void TightJPEGEncoder::writeSolidRect(int width, int height,
                                      const PixelFormat& pf,
                                      const rdr::U8* colour)
{
  // FIXME: Add a shortcut in the JPEG compressor to handle this case
  //        without having to use the default fallback which is very slow.
  Encoder::writeSolidRect(width, height, pf, colour);
}

void TightJPEGEncoder::writeCompact(rdr::U32 value, rdr::OutStream* os)
{
  // Copied from TightEncoder as it's overkill to inherit just for this
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
