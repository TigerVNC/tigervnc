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

//
// ZRLE decoding function.
//
// This file is #included after having set the following macro:
// BPP                - 8, 16 or 32

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#ifdef CPIXEL
#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL(is) CONCAT2E(readOpaque,CPIXEL)(is)
#define ZRLE_DECODE CONCAT2E(zrleDecode,CPIXEL)
#else
#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL(is) is->CONCAT2E(readOpaque,BPP)()
#define ZRLE_DECODE CONCAT2E(zrleDecode,BPP)
#endif

void ZRLE_DECODE (const Rect& r, rdr::InStream* is,
                  rdr::ZlibInStream* zis,
                  const PixelFormat& pf, ModifiablePixelBuffer* pb)
{
  int length = is->readU32();
  zis->setUnderlying(is, length);
  Rect t;
  PIXEL_T buf[64 * 64];

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 64) {

    t.br.y = __rfbmin(r.br.y, t.tl.y + 64);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 64) {

      t.br.x = __rfbmin(r.br.x, t.tl.x + 64);

      zlibHasData(zis, 1);
      int mode = zis->readU8();
      bool rle = mode & 128;
      int palSize = mode & 127;
      PIXEL_T palette[128];

#ifdef CPIXEL
      zlibHasData(zis, 3 * palSize);
#else
      zlibHasData(zis, BPP/8 * palSize);
#endif
      for (int i = 0; i < palSize; i++) {
        palette[i] = READ_PIXEL(zis);
      }

      if (palSize == 1) {
        PIXEL_T pix = palette[0];
        pb->fillRect(pf, t, &pix);
        continue;
      }

      if (!rle) {
        if (palSize == 0) {

          // raw

#ifdef CPIXEL
          zlibHasData(zis, 3 * t.area());
          for (PIXEL_T* ptr = buf; ptr < buf+t.area(); ptr++) {
            *ptr = READ_PIXEL(zis);
          }
#else
          zlibHasData(zis, BPP/8 * t.area());
          zis->readBytes(buf, t.area() * (BPP / 8));
#endif

        } else {

          // packed pixels
          int bppp = ((palSize > 16) ? 8 :
                      ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));

          PIXEL_T* ptr = buf;

          for (int i = 0; i < t.height(); i++) {
            PIXEL_T* eol = ptr + t.width();
            rdr::U8 byte = 0;
            rdr::U8 nbits = 0;

            while (ptr < eol) {
              if (nbits == 0) {
                zlibHasData(zis, 1);
                byte = zis->readU8();
                nbits = 8;
              }
              nbits -= bppp;
              rdr::U8 index = (byte >> nbits) & ((1 << bppp) - 1) & 127;
              *ptr++ = palette[index];
            }
          }
        }

      } else {

        if (palSize == 0) {

          // plain RLE

          PIXEL_T* ptr = buf;
          PIXEL_T* end = ptr + t.area();
          while (ptr < end) {
#ifdef CPIXEL
            zlibHasData(zis, 3);
#else
            zlibHasData(zis, BPP/8);
#endif
            PIXEL_T pix = READ_PIXEL(zis);
            int len = 1;
            int b;
            do {
              zlibHasData(zis, 1);
              b = zis->readU8();
              len += b;
            } while (b == 255);

            if (end - ptr < len) {
              throw Exception ("ZRLE decode error");
            }

            while (len-- > 0) *ptr++ = pix;

          }
        } else {

          // palette RLE

          PIXEL_T* ptr = buf;
          PIXEL_T* end = ptr + t.area();
          while (ptr < end) {
            zlibHasData(zis, 1);
            int index = zis->readU8();
            int len = 1;
            if (index & 128) {
              int b;
              do {
                zlibHasData(zis, 1);
                b = zis->readU8();
                len += b;
              } while (b == 255);

              if (end - ptr < len) {
                throw Exception ("ZRLE decode error");
              }
            }

            index &= 127;

            PIXEL_T pix = palette[index];

            while (len-- > 0) *ptr++ = pix;
          }
        }
      }

      pb->imageRect(pf, t, buf);
    }
  }

  zis->flushUnderlying();
  zis->setUnderlying(NULL, 0);
}

#undef ZRLE_DECODE
#undef READ_PIXEL
#undef PIXEL_T
}
