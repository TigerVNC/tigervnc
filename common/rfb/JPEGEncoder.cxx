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

  rdr::OutStream* os;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  jc.clear();
  jc.compress(buffer, stride, pb->getRect(), pb->getPF());

  os = conn->getOutStream();

  os->writeBytes(jc.data(), jc.length());
}

void JPEGEncoder::writeSolidRect(int width, int height,
                                 const PixelFormat& pf,
                                 const uint8_t* colour)
{
  // FIXME: Add a shortcut in the JPEG compressor to handle this case
  //        without having to use the default fallback which is very slow.
  Encoder::writeSolidRect(width, height, pf, colour);
}
