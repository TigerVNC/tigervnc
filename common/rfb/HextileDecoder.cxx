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

#include <algorithm>

#include <core/i18n.h>

#include <rdr/InStream.h>
#include <rdr/MemInStream.h>
#include <rdr/OutStream.h>

#include <rfb/Exception.h>
#include <rfb/ServerParams.h>
#include <rfb/PixelBuffer.h>
#include <rfb/HextileDecoder.h>
#include <rfb/hextileConstants.h>

using namespace rfb;

HextileDecoder::HextileDecoder()
  : Decoder(DecoderPlain), readTile(0, 0)
{
}

HextileDecoder::~HextileDecoder()
{
}

bool HextileDecoder::readRect(const core::Rect& r, rdr::InStream* is,
                              const ServerParams& server, rdr::OutStream* os)
{
  size_t bytesPerPixel;

  bytesPerPixel = server.pf().bpp/8;

  for (; readTile.y < r.height(); readTile.y += 16) {
    int height;

    height = std::min(r.height() - readTile.y, 16);

    for (; readTile.x < r.width(); readTile.x += 16) {
      int width;

      uint8_t tileType;
      uint8_t bg[4], fg[4];
      uint8_t nSubrects;

      width = std::min(r.width() - readTile.x, 16);

      if (!is->hasData(1))
        return false;

      is->setRestorePoint();

      tileType = is->readU8();

      if (tileType & hextileRaw) {
        if (!is->hasDataOrRestore(width * height * bytesPerPixel))
          return false;
        os->writeU8(tileType);
        os->copyBytes(is, width * height * bytesPerPixel);
        is->clearRestorePoint();
        continue;
      }

      if (tileType & hextileBgSpecified) {
        if (!is->hasDataOrRestore(bytesPerPixel))
          return false;
        is->readBytes(bg, bytesPerPixel);
      }

      if (tileType & hextileFgSpecified) {
        if (!is->hasDataOrRestore(bytesPerPixel))
          return false;
        is->readBytes(fg, bytesPerPixel);
      }

      nSubrects = 0;
      if (tileType & hextileAnySubrects) {
        if (!is->hasDataOrRestore(1))
          return false;

        nSubrects = is->readU8();

        if (tileType & hextileSubrectsColoured) {
          if (!is->hasDataOrRestore(nSubrects * (bytesPerPixel + 2)))
            return false;
        } else {
          if (!is->hasDataOrRestore(nSubrects * 2))
            return false;
        }
      }

      os->writeU8(tileType);
      if (tileType & hextileBgSpecified)
        os->writeBytes(bg, bytesPerPixel);
      if (tileType & hextileFgSpecified)
        os->writeBytes(fg, bytesPerPixel);
      if (tileType & hextileAnySubrects) {
        os->writeU8(nSubrects);
        if (tileType & hextileSubrectsColoured)
          os->copyBytes(is, nSubrects * (bytesPerPixel + 2));
        else
          os->copyBytes(is, nSubrects * 2);
      }

      is->clearRestorePoint();
    }

    readTile.x = 0;
  }

  readTile.y = 0;

  return true;
}

void HextileDecoder::decodeRect(const core::Rect& r, const uint8_t* buffer,
                                size_t buflen, const ServerParams& server,
                                ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  const PixelFormat& pf = server.pf();
  switch (pf.bpp) {
  case 8:  hextileDecode<uint8_t >(r, &is, pf, pb); break;
  case 16: hextileDecode<uint16_t>(r, &is, pf, pb); break;
  case 32: hextileDecode<uint32_t>(r, &is, pf, pb); break;
  }
}

template<class T>
inline T HextileDecoder::readPixel(rdr::InStream* is)
{
  if (sizeof(T) == 1)
    return is->readOpaque8();
  if (sizeof(T) == 2)
    return is->readOpaque16();
  if (sizeof(T) == 4)
    return is->readOpaque32();
}

template<class T>
void HextileDecoder::hextileDecode(const core::Rect& r, rdr::InStream* is,
                                   const PixelFormat& pf,
                                   ModifiablePixelBuffer* pb)
{
  core::Rect t;
  T bg = 0;
  T fg = 0;
  T buf[16 * 16];

  for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

    t.br.y = std::min(r.br.y, t.tl.y + 16);

    for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {

      t.br.x = std::min(r.br.x, t.tl.x + 16);

      int tileType = is->readU8();

      if (tileType & hextileRaw) {
        is->readBytes((uint8_t*)buf, t.area() * sizeof(T));
        pb->imageRect(pf, t, buf);
        continue;
      }

      if (tileType & hextileBgSpecified)
        bg = readPixel<T>(is);

      int len = t.area();
      T* ptr = buf;
      while (len-- > 0) *ptr++ = bg;

      if (tileType & hextileFgSpecified)
        fg = readPixel<T>(is);

      if (tileType & hextileAnySubrects) {
        int nSubrects = is->readU8();

        for (int i = 0; i < nSubrects; i++) {

          if (tileType & hextileSubrectsColoured)
            fg = readPixel<T>(is);

          int xy = is->readU8();
          int wh = is->readU8();

          int x = ((xy >> 4) & 15);
          int y = (xy & 15);
          int w = ((wh >> 4) & 15) + 1;
          int h = (wh & 15) + 1;
          if (x + w > 16 || y + h > 16) {
            throw protocol_error(_("Invalid Hextile coordinates"));
          }
          ptr = buf + y * t.width() + x;
          int rowAdd = t.width() - w;
          while (h-- > 0) {
            len = w;
            while (len-- > 0) *ptr++ = fg;
            ptr += rowAdd;
          }
        }
      }
      pb->imageRect(pf, t, buf);
    }
  }
}
