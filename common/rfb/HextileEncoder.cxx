/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Constantin Kaplinsky.  All Rights Reserved.
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
#include <rfb/encodings.h>
#include <rfb/SMsgWriter.h>
#include <rfb/SConnection.h>
#include <rfb/HextileEncoder.h>
#include <rfb/PixelFormat.h>
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

HextileEncoder::HextileEncoder(SConnection* conn) : Encoder(conn)
{
}

HextileEncoder::~HextileEncoder()
{
}

void HextileEncoder::writeRect(const Rect& r, PixelBuffer* pb)
{
  conn->writer()->startRect(r, encodingHextile);
  rdr::OutStream* os = conn->getOutStream();
  const PixelFormat& pf = conn->cp.pf();
  switch (pf.bpp) {
  case 8:
    if (improvedHextile) {
      hextileEncodeBetter8(r, os, pf, pb);
    } else {
      hextileEncode8(r, os, pf, pb);
    }
    break;
  case 16:
    if (improvedHextile) {
      hextileEncodeBetter16(r, os, pf, pb);
    } else {
      hextileEncode16(r, os, pf, pb);
    }
    break;
  case 32:
    if (improvedHextile) {
      hextileEncodeBetter32(r, os, pf, pb);
    } else {
      hextileEncode32(r, os, pf, pb);
    }
    break;
  }
  conn->writer()->endRect();
}
