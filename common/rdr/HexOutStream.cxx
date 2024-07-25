/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2019-2022 Pierre Ossman for Cendio AB
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
#include <rdr/HexOutStream.h>
#include <rfb/util.h>

using namespace rdr;

HexOutStream::HexOutStream(OutStream& os)
  : out_stream(os)
{
}

HexOutStream::~HexOutStream()
{
}

bool HexOutStream::flushBuffer()
{
  while (sentUpTo != ptr) {
    uint8_t* optr = out_stream.getptr(2);
    size_t length = std::min((size_t)(ptr-sentUpTo), out_stream.avail()/2);

    for (size_t i=0; i<length; i++)
      rfb::binToHex(&sentUpTo[i], 1, (char*)&optr[i*2], 2);

    out_stream.setptr(length*2);
    sentUpTo += length;
  }

  return true;
}

void
HexOutStream::flush() {
  BufferedOutStream::flush();
  out_stream.flush();
}

void HexOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);
  out_stream.cork(enable);
}
