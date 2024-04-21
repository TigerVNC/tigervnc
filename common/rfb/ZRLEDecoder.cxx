/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2022 Pierre Ossman for Cendio AB
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

static inline uint32_t readOpaque24A(rdr::InStream* is)
{
  uint32_t r=0;
  ((uint8_t*)&r)[0] = is->readU8();
  ((uint8_t*)&r)[1] = is->readU8();
  ((uint8_t*)&r)[2] = is->readU8();
  return r;

}
static inline uint32_t readOpaque24B(rdr::InStream* is)
{
  uint32_t r=0;
  ((uint8_t*)&r)[1] = is->readU8();
  ((uint8_t*)&r)[2] = is->readU8();
  ((uint8_t*)&r)[3] = is->readU8();
  return r;
}

template<class T>
static inline T readPixel(rdr::ZlibInStream* zis)
{
  if (sizeof(T) == 1)
    return zis->readOpaque8();
  if (sizeof(T) == 2)
    return zis->readOpaque16();
  if (sizeof(T) == 4)
    return zis->readOpaque32();
}

static inline void zlibHasData(rdr::ZlibInStream* zis, size_t length)
{
  if (!zis->hasData(length))
    throw Exception("ZRLE decode error");
}

ZRLEDecoder::ZRLEDecoder() : Decoder(DecoderOrdered)
{
}

ZRLEDecoder::~ZRLEDecoder()
{
}

bool ZRLEDecoder::readRect(const Rect& /*r*/, rdr::InStream* is,
                           const ServerParams& /*server*/,
                           rdr::OutStream* os)
{
  uint32_t len;

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

void ZRLEDecoder::decodeRect(const Rect& r, const uint8_t* buffer,
                             size_t buflen, const ServerParams& server,
                             ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const rfb::PixelFormat& pf = server.pf();
  switch (pf.bpp) {
  case 8:  zrleDecode<uint8_t>(r, &is, pf, pb); break;
  case 16: zrleDecode<uint16_t>(r, &is, pf, pb); break;
  case 32: zrleDecode<uint32_t>(r, &is, pf, pb); break;
  }
}

template<class T>
void ZRLEDecoder::zrleDecode(const Rect& r, rdr::InStream* is,
                             const PixelFormat& pf,
                             ModifiablePixelBuffer* pb)
{
  int length = is->readU32();
  zis.setUnderlying(is, length);
  Rect t;
  T buf[64 * 64];

  Pixel maxPixel = pf.pixelFromRGB((uint16_t)-1, (uint16_t)-1, (uint16_t)-1);
  bool fitsInLS3Bytes = maxPixel < (1<<24);
  bool fitsInMS3Bytes = (maxPixel & 0xff) == 0;
  bool isLowCPixel = (sizeof(T) == 4) &&
                     ((fitsInLS3Bytes && pf.isLittleEndian()) ||
                      (fitsInMS3Bytes && pf.isBigEndian()));
  bool isHighCPixel = (sizeof(T) == 4) &&
                      ((fitsInLS3Bytes && pf.isBigEndian()) ||
                       (fitsInMS3Bytes && pf.isLittleEndian()));

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 64) {

    t.br.y = __rfbmin(r.br.y, t.tl.y + 64);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 64) {

      t.br.x = __rfbmin(r.br.x, t.tl.x + 64);

      zlibHasData(&zis, 1);
      int mode = zis.readU8();
      bool rle = mode & 128;
      int palSize = mode & 127;
      T palette[128];

      if (isLowCPixel || isHighCPixel)
        zlibHasData(&zis, 3 * palSize);
      else
        zlibHasData(&zis, sizeof(T) * palSize);

      for (int i = 0; i < palSize; i++) {
        if (isLowCPixel)
          palette[i] = readOpaque24A(&zis);
        else if (isHighCPixel)
          palette[i] = readOpaque24B(&zis);
        else
          palette[i] = readPixel<T>(&zis);
      }

      if (palSize == 1) {
        T pix = palette[0];
        pb->fillRect(pf, t, &pix);
        continue;
      }

      if (!rle) {
        if (palSize == 0) {

          // raw

          if (isLowCPixel || isHighCPixel)
            zlibHasData(&zis, 3 * t.area());
          else
            zlibHasData(&zis, sizeof(T) * t.area());

          if (isLowCPixel || isHighCPixel) {
            for (T* ptr = buf; ptr < buf+t.area(); ptr++) {
              if (isLowCPixel)
                *ptr = readOpaque24A(&zis);
              else
                *ptr = readOpaque24B(&zis);
            }
          } else {
            zis.readBytes((uint8_t*)buf, t.area() * sizeof(T));
          }

        } else {

          // packed pixels
          int bppp = ((palSize > 16) ? 8 :
                      ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));

          T* ptr = buf;

          for (int i = 0; i < t.height(); i++) {
            T* eol = ptr + t.width();
            uint8_t byte = 0;
            uint8_t nbits = 0;

            while (ptr < eol) {
              if (nbits == 0) {
                zlibHasData(&zis, 1);
                byte = zis.readU8();
                nbits = 8;
              }
              nbits -= bppp;
              uint8_t index = (byte >> nbits) & ((1 << bppp) - 1) & 127;
              *ptr++ = palette[index];
            }
          }
        }

      } else {

        if (palSize == 0) {

          // plain RLE

          T* ptr = buf;
          T* end = ptr + t.area();
          while (ptr < end) {
            T pix;
            if (isLowCPixel || isHighCPixel)
              zlibHasData(&zis, 3);
            else
              zlibHasData(&zis, sizeof(T));
            if (isLowCPixel)
              pix = readOpaque24A(&zis);
            else if (isHighCPixel)
              pix = readOpaque24B(&zis);
            else
              pix = readPixel<T>(&zis);
            int len = 1;
            int b;
            do {
              zlibHasData(&zis, 1);
              b = zis.readU8();
              len += b;
            } while (b == 255);

            if (end - ptr < len) {
              throw Exception ("ZRLE decode error");
            }

            while (len-- > 0) *ptr++ = pix;

          }
        } else {

          // palette RLE

          T* ptr = buf;
          T* end = ptr + t.area();
          while (ptr < end) {
            zlibHasData(&zis, 1);
            int index = zis.readU8();
            int len = 1;
            if (index & 128) {
              int b;
              do {
                zlibHasData(&zis, 1);
                b = zis.readU8();
                len += b;
              } while (b == 255);

              if (end - ptr < len) {
                throw Exception ("ZRLE decode error");
              }
            }

            index &= 127;

            T pix = palette[index];

            while (len-- > 0) *ptr++ = pix;
          }
        }
      }

      pb->imageRect(pf, t, buf);
    }
  }

  zis.flushUnderlying();
  zis.setUnderlying(nullptr, 0);
}
