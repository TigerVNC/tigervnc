/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Constantin Kaplinsky.  All Rights Reserved.
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

#include <rfb/encodings.h>
#include <rfb/SConnection.h>
#include <rfb/HextileEncoder.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Configuration.h>

using namespace rfb;

BoolParameter improvedHextile("ImprovedHextile",
                              "Use improved compression algorithm for Hextile "
                              "encoding which achieves better compression "
                              "ratios by the cost of using more CPU time",
                              true);

#define BPP 8
#include <rfb/hextileEncode.h>
#include <rfb/hextileEncodeBetter.h>
#undef BPP
#define BPP 16
#include <rfb/hextileEncode.h>
#include <rfb/hextileEncodeBetter.h>
#undef BPP
#define BPP 32
#include <rfb/hextileEncode.h>
#include <rfb/hextileEncodeBetter.h>
#undef BPP

HextileEncoder::HextileEncoder(SConnection* conn) :
  Encoder(conn, encodingHextile, EncoderPlain)
{
}

HextileEncoder::~HextileEncoder()
{
}

bool HextileEncoder::isSupported()
{
  return conn->client.supportsEncoding(encodingHextile);
}

void HextileEncoder::writeRect(const PixelBuffer* pb, const Palette& palette)
{
  rdr::OutStream* os = conn->getOutStream();
  switch (pb->getPF().bpp) {
  case 8:
    if (improvedHextile) {
      hextileEncodeBetter8(os, pb);
    } else {
      hextileEncode8(os, pb);
    }
    break;
  case 16:
    if (improvedHextile) {
      hextileEncodeBetter16(os, pb);
    } else {
      hextileEncode16(os, pb);
    }
    break;
  case 32:
    if (improvedHextile) {
      hextileEncodeBetter32(os, pb);
    } else {
      hextileEncode32(os, pb);
    }
    break;
  }
}

void HextileEncoder::writeSolidRect(int width, int height,
                                    const PixelFormat& pf,
                                    const rdr::U8* colour)
{
  rdr::OutStream* os;
  int tiles;

  os = conn->getOutStream();

  tiles = ((width + 15)/16) * ((height + 15)/16);

  os->writeU8(hextileBgSpecified);
  os->writeBytes(colour, pf.bpp/8);
  tiles--;

  while (tiles--)
      os->writeU8(0);
}
