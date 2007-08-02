/* Copyright (C) 2007 Constantin Kaplinsky.  All Rights Reserved.
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

#include <rfb/JpegEncoder.h>
#include <rdr/OutStream.h>
#include <rfb/encodings.h>

using namespace rfb;

const int JpegEncoder::qualityMap[10] = {
  5, 10, 15, 25, 37, 50, 60, 70, 75, 80
};

JpegEncoder::JpegEncoder(SMsgWriter* writer_) : writer(writer_)
{
  jcomp = new StandardJpegCompressor;
  jcomp->setQuality(qualityMap[6]);
}

JpegEncoder::~JpegEncoder()
{
  delete jcomp;
}

void JpegEncoder::setQualityLevel(int level)
{
  if (level >= 0 && level <= 9) {
    jcomp->setQuality(qualityMap[level]);
  }
}

bool JpegEncoder::writeRect(PixelBuffer* pb, const Rect& r)
{
  int serverBitsPerPixel = pb->getPF().bpp;
  int clientBitsPerPixel = writer->bpp();

  // FIXME: Implement JPEG compression for (serverBitsPerPixel == 16).
  // FIXME: Check that all color components are actually 8 bits wide.
  if (serverBitsPerPixel != 32 || clientBitsPerPixel < 16) {
    // FIXME: Make sure this return value is checked properly.
    return false;
  }

  writer->startRect(r, encodingTight);
  rdr::OutStream* os = writer->getOutStream();

  // Get access to pixel data
  int stride;
  const rdr::U32* pixels = (const rdr::U32 *)pb->getPixelsR(r, &stride);
  const PixelFormat& fmt = pb->getPF();

  // Encode data
  jcomp->compress(pixels, &fmt, r.width(), r.height(), stride);

  // Write Tight-encoded header and JPEG data.
  os->writeU8(0x09 << 4);
  os->writeCompactLength(jcomp->getDataLength());
  os->writeBytes(jcomp->getDataPtr(), jcomp->getDataLength());

  writer->endRect();

  return true;
}

