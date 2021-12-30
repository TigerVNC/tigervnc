/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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

#include <rfb/Encoder.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Palette.h>

using namespace rfb;

Encoder::Encoder(SConnection *conn_, int encoding_,
                 enum EncoderFlags flags_,
                 unsigned int maxPaletteSize_, int losslessQuality_) :
  encoding(encoding_), flags(flags_),
  maxPaletteSize(maxPaletteSize_), losslessQuality(losslessQuality_),
  conn(conn_)
{
}

Encoder::~Encoder()
{
}

void Encoder::writeSolidRect(int width, int height,
                             const PixelFormat& pf, const rdr::U8* colour)
{
  ManagedPixelBuffer buffer(pf, width, height);

  Palette palette;
  rdr::U32 palcol;

  buffer.fillRect(buffer.getRect(), colour);

  palcol = 0;
  memcpy(&palcol, colour, pf.bpp/8);
  palette.insert(palcol, 1);

  writeRect(&buffer, palette);
}

void Encoder::writeSolidRect(const PixelBuffer* pb, const Palette& palette)
{
  rdr::U32 col32;
  rdr::U16 col16;
  rdr::U8 col8;

  rdr::U8* buffer;

  assert(palette.size() == 1);

  // The Palette relies on implicit up and down conversion
  switch (pb->getPF().bpp) {
  case 32:
    col32 = (rdr::U32)palette.getColour(0);
    buffer = (rdr::U8*)&col32;
    break;
  case 16:
    col16 = (rdr::U16)palette.getColour(0);
    buffer = (rdr::U8*)&col16;
    break;
  default:
    col8 = (rdr::U8)palette.getColour(0);
    buffer = (rdr::U8*)&col8;
    break;
  }

  writeSolidRect(pb->width(), pb->height(), pb->getPF(), buffer);
}
