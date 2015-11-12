/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <rdr/InStream.h>
#include <rdr/MemInStream.h>
#include <rdr/OutStream.h>

#include <rfb/ConnParams.h>
#include <rfb/PixelBuffer.h>
#include <rfb/HextileDecoder.h>

using namespace rfb;

#define BPP 8
#include <rfb/hextileDecode.h>
#undef BPP
#define BPP 16
#include <rfb/hextileDecode.h>
#undef BPP
#define BPP 32
#include <rfb/hextileDecode.h>
#undef BPP

HextileDecoder::HextileDecoder() : Decoder(DecoderPlain)
{
}

HextileDecoder::~HextileDecoder()
{
}

void HextileDecoder::readRect(const Rect& r, rdr::InStream* is,
                              const ConnParams& cp, rdr::OutStream* os)
{
  Rect t;
  size_t bytesPerPixel;

  bytesPerPixel = cp.pf().bpp/8;

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

    t.br.y = __rfbmin(r.br.y, t.tl.y + 16);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {
      rdr::U8 tileType;

      t.br.x = __rfbmin(r.br.x, t.tl.x + 16);

      tileType = is->readU8();
      os->writeU8(tileType);

      if (tileType & hextileRaw) {
        os->copyBytes(is, t.area() * bytesPerPixel);
        continue;
      }

      if (tileType & hextileBgSpecified)
        os->copyBytes(is, bytesPerPixel);

      if (tileType & hextileFgSpecified)
        os->copyBytes(is, bytesPerPixel);

      if (tileType & hextileAnySubrects) {
        rdr::U8 nSubrects;

        nSubrects = is->readU8();
        os->writeU8(nSubrects);

        if (tileType & hextileSubrectsColoured)
          os->copyBytes(is, nSubrects * (bytesPerPixel + 2));
        else
          os->copyBytes(is, nSubrects * 2);
      }
    }
  }
}

void HextileDecoder::decodeRect(const Rect& r, const void* buffer,
                                size_t buflen, const ConnParams& cp,
                                ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const PixelFormat& pf = cp.pf();
  switch (pf.bpp) {
  case 8:  hextileDecode8 (r, &is, pf, pb); break;
  case 16: hextileDecode16(r, &is, pf, pb); break;
  case 32: hextileDecode32(r, &is, pf, pb); break;
  }
}
