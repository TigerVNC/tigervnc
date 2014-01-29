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
#include <rdr/OutStream.h>
#include <rfb/Exception.h>
#include <rfb/TransImageGetter.h>
#include <rfb/encodings.h>
#include <rfb/ConnParams.h>
#include <rfb/SMsgWriter.h>
#include <rfb/ZRLEEncoder.h>
#include <rfb/Configuration.h>

using namespace rfb;

IntParameter zlibLevel("ZlibLevel","Zlib compression level",-1);

#define EXTRA_ARGS ImageGetter* ig
#define GET_IMAGE_INTO_BUF(r,buf) ig->getImage(buf, r);
#define BPP 8
#include <rfb/zrleEncode.h>
#undef BPP
#define BPP 16
#include <rfb/zrleEncode.h>
#undef BPP
#define BPP 32
#include <rfb/zrleEncode.h>
#define CPIXEL 24A
#include <rfb/zrleEncode.h>
#undef CPIXEL
#define CPIXEL 24B
#include <rfb/zrleEncode.h>
#undef CPIXEL
#undef BPP

ZRLEEncoder::ZRLEEncoder(SMsgWriter* writer)
  : Encoder(writer), zos(0,0,zlibLevel), mos(129*1024)
{
}

ZRLEEncoder::~ZRLEEncoder()
{
}

void ZRLEEncoder::writeRect(const Rect& r, TransImageGetter* ig)
{
  rdr::U8* imageBuf = writer->getImageBuf(64 * 64 * 4 + 4);
  mos.clear();

  switch (writer->bpp()) {
  case 8:
    zrleEncode8(r, &mos, &zos, imageBuf, ig);
    break;
  case 16:
    zrleEncode16(r, &mos, &zos, imageBuf, ig);
    break;
  case 32:
    {
      const PixelFormat& pf = writer->getConnParams()->pf();

      Pixel maxPixel = pf.pixelFromRGB((rdr::U16)-1, (rdr::U16)-1, (rdr::U16)-1);
      bool fitsInLS3Bytes = maxPixel < (1<<24);
      bool fitsInMS3Bytes = (maxPixel & 0xff) == 0;

      if ((fitsInLS3Bytes && pf.isLittleEndian()) ||
          (fitsInMS3Bytes && pf.isBigEndian()))
      {
        zrleEncode24A(r, &mos, &zos, imageBuf, ig);
      }
      else if ((fitsInLS3Bytes && pf.isBigEndian()) ||
               (fitsInMS3Bytes && pf.isLittleEndian()))
      {
        zrleEncode24B(r, &mos, &zos, imageBuf, ig);
      }
      else
      {
        zrleEncode32(r, &mos, &zos, imageBuf, ig);
      }
      break;
    }
  }

  writer->startRect(r, encodingZRLE);
  rdr::OutStream* os = writer->getOutStream();
  os->writeU32(mos.length());
  os->writeBytes(mos.data(), mos.length());
  writer->endRect();
}
