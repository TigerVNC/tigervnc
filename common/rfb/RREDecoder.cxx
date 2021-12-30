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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rdr/InStream.h>
#include <rdr/MemInStream.h>
#include <rdr/OutStream.h>

#include <rfb/ServerParams.h>
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

bool RREDecoder::readRect(const Rect& r, rdr::InStream* is,
                          const ServerParams& server, rdr::OutStream* os)
{
  rdr::U32 numRects;
  size_t len;

  if (!is->hasData(4))
    return false;

  is->setRestorePoint();

  numRects = is->readU32();
  os->writeU32(numRects);

  len = server.pf().bpp/8 + numRects * (server.pf().bpp/8 + 8);

  if (!is->hasDataOrRestore(len))
    return false;

  is->clearRestorePoint();

  os->copyBytes(is, len);

  return true;
}

void RREDecoder::decodeRect(const Rect& r, const void* buffer,
                            size_t buflen, const ServerParams& server,
                            ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const PixelFormat& pf = server.pf();
  switch (pf.bpp) {
  case 8:  rreDecode8 (r, &is, pf, pb); break;
  case 16: rreDecode16(r, &is, pf, pb); break;
  case 32: rreDecode32(r, &is, pf, pb); break;
  }
}
