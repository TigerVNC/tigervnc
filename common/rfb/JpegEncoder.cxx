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

#ifdef HAVE_COMMON_CONFIG_H
#include <common-config.h>
#endif

#include <rfb/JpegEncoder.h>
#include <rdr/OutStream.h>
#include <rdr/Exception.h>
#include <rfb/encodings.h>
#include <rfb/ConnParams.h>
#include <rfb/LogWriter.h>

#ifdef HAVE_CL
#include <rfb/IrixCLJpegCompressor.h>
#endif
#ifdef HAVE_DMEDIA
#include <rfb/IrixDMJpegCompressor.h>
#endif

using namespace rfb;

static LogWriter vlog("JpegEncoder");

BoolParameter JpegEncoder::useHardwareJPEG
("UseHardwareJPEG",
 "Use hardware-accelerated JPEG compressor for video if available",
 true);

const int JpegEncoder::qualityMap[10] = {
  2, 10, 15, 25, 37, 50, 60, 70, 80, 90
};

JpegEncoder::JpegEncoder(SMsgWriter* writer_) : writer(writer_), jcomp(0)
{
#ifdef HAVE_DMEDIA
#ifdef DEBUG_FORCE_CL
    vlog.debug("DEBUG: skipping IRIX DM JPEG compressor");
#else
  if (!jcomp && useHardwareJPEG) {
    vlog.debug("trying IRIX DM JPEG compressor");
    IrixDMJpegCompressor *dmComp = new IrixDMJpegCompressor;
    if (dmComp->isValid()) {
      vlog.debug("initialized IRIX DM JPEG compressor successfully");
      jcomp = dmComp;
    } else {
      vlog.error("warning: could not create IRIX DM JPEG compressor");
      delete dmComp;
    }
  }
#endif
#endif

#ifdef HAVE_CL
  if (!jcomp && useHardwareJPEG) {
    vlog.debug("trying IRIX CL JPEG compressor");
    IrixCLJpegCompressor *clComp = new IrixCLJpegCompressor;
    if (clComp->isValid()) {
      vlog.debug("initialized IRIX CL JPEG compressor successfully");
      jcomp = clComp;
    } else {
      vlog.error("warning: could not create IRIX CL JPEG compressor");
      delete clComp;
    }
  }
#endif

  if (!jcomp) {
    if (useHardwareJPEG) {
      vlog.info("no hardware JPEG compressor available");
    }
    vlog.debug("using software JPEG compressor");
    jcomp = new StandardJpegCompressor;
  }
  jcomp->setQuality(qualityMap[6]);
}

JpegEncoder::~JpegEncoder()
{
  delete jcomp;
}

void JpegEncoder::setQualityLevel(int level)
{
  if (level < 0) {
    level = 0;
  } else if (level > 9) {
    level = 9;
  }
  jcomp->setQuality(qualityMap[level]);
}

bool JpegEncoder::isPixelFormatSupported(PixelBuffer* pb) const
{
  const PixelFormat &serverPF = pb->getPF();
  const PixelFormat &clientPF = writer->getConnParams()->pf();

  // FIXME: Ask encoders if they support given pixel formats.

  if ( serverPF.bpp == 32 && clientPF.bpp >= 16 &&
       serverPF.depth == 24 && serverPF.redMax == 255 &&
       serverPF.greenMax == 255 && serverPF.blueMax  == 255 ) {
    return true;
  }

  return false;
}

void JpegEncoder::writeRect(PixelBuffer* pb, const Rect& r)
{
  if (!isPixelFormatSupported(pb)) {
    vlog.error("pixel format unsupported by JPEG encoder");
    throw rdr::Exception("internal error in JpegEncoder");
  }

  writer->startRect(r, encodingTight);
  rdr::OutStream* os = writer->getOutStream();

  // Get access to pixel data
  int stride;
  const rdr::U32* pixels = (const rdr::U32 *)pb->getPixelsR(r, &stride);
  const PixelFormat& fmt = pb->getPF();

  // Try to encode data
  jcomp->compress(pixels, &fmt, r.width(), r.height(), stride);

  // If not successful, switch to StandardJpegCompressor
  if (jcomp->getDataLength() == 0) {
    vlog.info("switching to standard software JPEG compressor");
    delete jcomp;
    jcomp = new StandardJpegCompressor;
    jcomp->setQuality(qualityMap[6]);
    jcomp->compress(pixels, &fmt, r.width(), r.height(), stride);
  }

  // Write Tight-encoded header and JPEG data.
  os->writeU8(0x09 << 4);
  os->writeCompactLength(jcomp->getDataLength());
  os->writeBytes(jcomp->getDataPtr(), jcomp->getDataLength());

  writer->endRect();
}

