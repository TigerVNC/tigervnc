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
#include <rdr/HexInStream.h>
#include <rdr/Exception.h>
#include <rfb/util.h>

using namespace rdr;

HexInStream::HexInStream(InStream& is)
: in_stream(is)
{
}

HexInStream::~HexInStream() {
}

bool HexInStream::fillBuffer() {
  if (!in_stream.hasData(2))
    return false;

  size_t length = std::min(in_stream.avail()/2, availSpace());
  const uint8_t* iptr = in_stream.getptr(length*2);

  uint8_t* optr = (uint8_t*) end;
  for (size_t i=0; i<length; i++) {
    if (!rfb::hexToBin((const char*)&iptr[i*2], 2, &optr[i], 1))
      throw Exception("HexInStream: Invalid input data");
  }

  in_stream.setptr(length*2);
  end += length;

  return true;
}
