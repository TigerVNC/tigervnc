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
#include <rfb/RREDecoder.h>

using namespace rfb;

#define BPP 8
#include <rfb/rreDecode.h>
#undef BPP
#define BPP 16
#include <rfb/rreDecode.h>
#undef BPP
#define BPP 32
#include <rfb/rreDecode.h>
#undef BPP

RREDecoder::RREDecoder() : Decoder(DecoderPlain)
{
}

RREDecoder::~RREDecoder()
{
}

void RREDecoder::readRect(const Rect& r, rdr::InStream* is,
                          const ConnParams& cp, rdr::OutStream* os)
{
  rdr::U32 numRects;

  numRects = is->readU32();
  os->writeU32(numRects);

  os->copyBytes(is, cp.pf().bpp/8 + numRects * (cp.pf().bpp/8 + 8));
}

void RREDecoder::decodeRect(const Rect& r, const void* buffer,
                            size_t buflen, const ConnParams& cp,
                            ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const PixelFormat& pf = cp.pf();
  switch (pf.bpp) {
  case 8:  rreDecode8 (r, &is, pf, pb); break;
  case 16: rreDecode16(r, &is, pf, pb); break;
  case 32: rreDecode32(r, &is, pf, pb); break;
  }
}
