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
#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/PixelFormat.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Palette.h>
#include <rfb/RREEncoder.h>

using namespace rfb;

#define BPP 8
#include <rfb/rreEncode.h>
#undef BPP
#define BPP 16
#include <rfb/rreEncode.h>
#undef BPP
#define BPP 32
#include <rfb/rreEncode.h>
#undef BPP

RREEncoder::RREEncoder(SConnection* conn) :
  Encoder(conn, encodingRRE, EncoderPlain)
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
  rdr::U8* imageBuf;
  int stride;
  rdr::U32 bg;

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
    nSubrects = rreEncode8((rdr::U8*)imageBuf, w, h, &mos, bg);
    break;
  case 16:
    nSubrects = rreEncode16((rdr::U16*)imageBuf, w, h, &mos, bg);
    break;
  case 32:
    nSubrects = rreEncode32((rdr::U32*)imageBuf, w, h, &mos, bg);
    break;
  }

  bufferCopy.commitBufferRW(pb->getRect());

  rdr::OutStream* os = conn->getOutStream();
  os->writeU32(nSubrects);
  os->writeBytes(mos.data(), mos.length());
  mos.clear();
}

void RREEncoder::writeSolidRect(int width, int height,
                                const PixelFormat& pf,
                                const rdr::U8* colour)
{
  rdr::OutStream* os;

  os = conn->getOutStream();

  os->writeU32(0);
  os->writeBytes(colour, pf.bpp/8);
}
