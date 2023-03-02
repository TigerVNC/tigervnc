/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2022 Pierre Ossman for Cendio AB
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

#include <rfb/Exception.h>
#include <rfb/ServerParams.h>
#include <rfb/PixelBuffer.h>
#include <rfb/RREDecoder.h>

using namespace rfb;

RREDecoder::RREDecoder() : Decoder(DecoderPlain)
{
}

RREDecoder::~RREDecoder()
{
}

bool RREDecoder::readRect(const Rect& /*r*/, rdr::InStream* is,
                          const ServerParams& server, rdr::OutStream* os)
{
  uint32_t numRects;
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

void RREDecoder::decodeRect(const Rect& r, const uint8_t* buffer,
                            size_t buflen, const ServerParams& server,
                            ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const PixelFormat& pf = server.pf();
  switch (pf.bpp) {
  case 8:  rreDecode<uint8_t >(r, &is, pf, pb); break;
  case 16: rreDecode<uint16_t>(r, &is, pf, pb); break;
  case 32: rreDecode<uint32_t>(r, &is, pf, pb); break;
  }
}

template<class T>
inline T RREDecoder::readPixel(rdr::InStream* is)
{
  if (sizeof(T) == 1)
    return is->readOpaque8();
  if (sizeof(T) == 2)
    return is->readOpaque16();
  if (sizeof(T) == 4)
    return is->readOpaque32();
}

template<class T>
void RREDecoder::rreDecode(const Rect& r, rdr::InStream* is,
                           const PixelFormat& pf,
                           ModifiablePixelBuffer* pb)
{
  int nSubrects = is->readU32();
  T bg = readPixel<T>(is);
  pb->fillRect(pf, r, &bg);

  for (int i = 0; i < nSubrects; i++) {
    T pix = readPixel<T>(is);
    int x = is->readU16();
    int y = is->readU16();
    int w = is->readU16();
    int h = is->readU16();

    if (((x+w) > r.width()) || ((y+h) > r.height()))
      throw Exception ("RRE decode error");

    pb->fillRect(pf, Rect(r.tl.x+x, r.tl.y+y, r.tl.x+x+w, r.tl.y+y+h), &pix);
  }
}
