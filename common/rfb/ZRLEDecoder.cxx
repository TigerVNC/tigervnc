/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2017 Pierre Ossman for Cendio AB
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
#include <rfb/ZRLEDecoder.h>

using namespace rfb;

static inline rdr::U32 readOpaque24A(rdr::InStream* is)
{
  rdr::U32 r=0;
  ((rdr::U8*)&r)[0] = is->readU8();
  ((rdr::U8*)&r)[1] = is->readU8();
  ((rdr::U8*)&r)[2] = is->readU8();
  return r;

}
static inline rdr::U32 readOpaque24B(rdr::InStream* is)
{
  rdr::U32 r=0;
  ((rdr::U8*)&r)[1] = is->readU8();
  ((rdr::U8*)&r)[2] = is->readU8();
  ((rdr::U8*)&r)[3] = is->readU8();
  return r;
}

static inline void zlibHasData(rdr::ZlibInStream* zis, size_t length)
{
  if (!zis->hasData(length))
    throw Exception("ZRLE decode error");
}

#define BPP 8
#include <rfb/zrleDecode.h>
#undef BPP
#define BPP 16
#include <rfb/zrleDecode.h>
#undef BPP
#define BPP 32
#include <rfb/zrleDecode.h>
#define CPIXEL 24A
#include <rfb/zrleDecode.h>
#undef CPIXEL
#define CPIXEL 24B
#include <rfb/zrleDecode.h>
#undef CPIXEL
#undef BPP

ZRLEDecoder::ZRLEDecoder() : Decoder(DecoderOrdered)
{
}

ZRLEDecoder::~ZRLEDecoder()
{
}

bool ZRLEDecoder::readRect(const Rect& r, rdr::InStream* is,
                           const ServerParams& server, rdr::OutStream* os)
{
  rdr::U32 len;

  if (!is->hasData(4))
    return false;

  is->setRestorePoint();

  len = is->readU32();
  os->writeU32(len);

  if (!is->hasDataOrRestore(len))
    return false;

  is->clearRestorePoint();

  os->copyBytes(is, len);

  return true;
}

void ZRLEDecoder::decodeRect(const Rect& r, const void* buffer,
                             size_t buflen, const ServerParams& server,
                             ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const rfb::PixelFormat& pf = server.pf();
  switch (pf.bpp) {
  case 8:  zrleDecode8 (r, &is, &zis, pf, pb); break;
  case 16: zrleDecode16(r, &is, &zis, pf, pb); break;
  case 32:
    {
      if (pf.depth <= 24) {
        Pixel maxPixel = pf.pixelFromRGB((rdr::U16)-1, (rdr::U16)-1, (rdr::U16)-1);
        bool fitsInLS3Bytes = maxPixel < (1<<24);
        bool fitsInMS3Bytes = (maxPixel & 0xff) == 0;

        if ((fitsInLS3Bytes && pf.isLittleEndian()) ||
            (fitsInMS3Bytes && pf.isBigEndian()))
        {
          zrleDecode24A(r, &is, &zis, pf, pb);
          break;
        }

        if ((fitsInLS3Bytes && pf.isBigEndian()) ||
            (fitsInMS3Bytes && pf.isLittleEndian()))
        {
          zrleDecode24B(r, &is, &zis, pf, pb);
          break;
        }
      }

      zrleDecode32(r, &is, &zis, pf, pb);
      break;
    }
  }
}
