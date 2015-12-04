/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#include <rfb/ConnParams.h>
#include <rfb/PixelBuffer.h>
#include <rfb/TightDecoder.h>

using namespace rfb;

#define TIGHT_MAX_WIDTH 2048

#define BPP 8
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 16
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 32
#include <rfb/tightDecode.h>
#undef BPP

TightDecoder::TightDecoder()
{
}

TightDecoder::~TightDecoder()
{
}

void TightDecoder::readRect(const Rect& r, rdr::InStream* is,
                            const ConnParams& cp, ModifiablePixelBuffer* pb)
{
  this->is = is;
  this->pb = pb;
  clientpf = pb->getPF();
  serverpf = cp.pf();

  if (clientpf.equal(serverpf)) {
    /* Decode directly into the framebuffer (fast path) */
    directDecode = true;
  } else {
    /* Decode into an intermediate buffer and use pixel translation */
    directDecode = false;
  }

  switch (serverpf.bpp) {
  case 8:
    tightDecode8 (r); break;
  case 16:
    tightDecode16(r); break;
  case 32:
    tightDecode32(r); break;
  }
}

rdr::U32 TightDecoder::readCompact(rdr::InStream* is)
{
  rdr::U8 b;
  rdr::U32 result;

  b = is->readU8();
  result = (int)b & 0x7F;
  if (b & 0x80) {
    b = is->readU8();
    result |= ((int)b & 0x7F) << 7;
    if (b & 0x80) {
      b = is->readU8();
      result |= ((int)b & 0xFF) << 14;
    }
  }

  return result;
}
