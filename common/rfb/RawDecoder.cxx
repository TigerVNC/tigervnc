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
#include <rfb/ConnParams.h>
#include <rfb/PixelBuffer.h>
#include <rfb/RawDecoder.h>

using namespace rfb;

RawDecoder::RawDecoder()
{
}

RawDecoder::~RawDecoder()
{
}

void RawDecoder::readRect(const Rect& r, rdr::InStream* is,
                          const ConnParams& cp, ModifiablePixelBuffer* pb)
{
  const PixelFormat& pf = cp.pf();

  rdr::U8 imageBuf[16384];
  const int maxPixels = sizeof(imageBuf) / (pf.bpp/8);

  int x = r.tl.x;
  int y = r.tl.y;
  int w = r.width();
  int h = r.height();

  while (h > 0) {
    int dx;

    dx = 0;
    while (dx < w) {
      int dw;

      dw = maxPixels;
      if (dx + dw > w)
        dw = w - dx;

      is->readBytes(imageBuf, dw * pf.bpp/8);
      pb->imageRect(pf, Rect(x+dx, y, x+dx+dw, y+1), imageBuf);

      dx += dw;
    }

    y++;
    h--;
  }
}
