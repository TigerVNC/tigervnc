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
#include <rfb/encodings.h>
#include <rfb/SMsgWriter.h>
#include <rfb/SConnection.h>
#include <rfb/PixelBuffer.h>
#include <rfb/RawEncoder.h>

using namespace rfb;

RawEncoder::RawEncoder(SConnection* conn) : Encoder(conn)
{
}

RawEncoder::~RawEncoder()
{
}

void RawEncoder::writeRect(const Rect& r, PixelBuffer* pb)
{
  rdr::U8* buf = conn->writer()->getImageBuf(r.area());

  pb->getImage(conn->cp.pf(), buf, r);

  conn->writer()->startRect(r, encodingRaw);
  conn->getOutStream()->writeBytes(buf, r.area() * conn->cp.pf().bpp/8);
  conn->writer()->endRect();
}
