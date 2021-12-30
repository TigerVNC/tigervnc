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

#include <assert.h>

#include <rdr/OutStream.h>
#include <rfb/ServerParams.h>
#include <rfb/PixelBuffer.h>
#include <rfb/RawDecoder.h>

using namespace rfb;

RawDecoder::RawDecoder() : Decoder(DecoderPlain)
{
}

RawDecoder::~RawDecoder()
{
}

bool RawDecoder::readRect(const Rect& r, rdr::InStream* is,
                          const ServerParams& server, rdr::OutStream* os)
{
  if (!is->hasData(r.area() * (server.pf().bpp/8)))
    return false;
  os->copyBytes(is, r.area() * (server.pf().bpp/8));
  return true;
}

void RawDecoder::decodeRect(const Rect& r, const void* buffer,
                            size_t buflen, const ServerParams& server,
                            ModifiablePixelBuffer* pb)
{
  assert(buflen >= (size_t)r.area() * (server.pf().bpp/8));
  pb->imageRect(server.pf(), r, buffer);
}
