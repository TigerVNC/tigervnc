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

#include <rdr/HexInStream.h>
#include <rdr/Exception.h>

#include <stdlib.h>
#include <ctype.h>

using namespace rdr;

static inline int min(int a, int b) {return a<b ? a : b;}

HexInStream::HexInStream(InStream& is)
: in_stream(is)
{
}

HexInStream::~HexInStream() {
}


bool HexInStream::readHexAndShift(char c, int* v) {
  c=tolower(c);
  if ((c >= '0') && (c <= '9'))
    *v = (*v << 4) + (c - '0');
  else if ((c >= 'a') && (c <= 'f'))
    *v = (*v << 4) + (c - 'a' + 10);
  else
    return false;
  return true;
}

bool HexInStream::hexStrToBin(const char* s, char** data, size_t* length) {
  size_t l=strlen(s);
  if ((l % 2) == 0) {
    delete [] *data;
    *data = 0; *length = 0;
    if (l == 0)
      return true;
    *data = new char[l/2];
    *length = l/2;
    for(size_t i=0;i<l;i+=2) {
      int byte = 0;
      if (!readHexAndShift(s[i], &byte) ||
        !readHexAndShift(s[i+1], &byte))
        goto decodeError;
      (*data)[i/2] = byte;
    }
    return true;
  }
decodeError:
  delete [] *data;
  *data = 0;
  *length = 0;
  return false;
}


bool HexInStream::fillBuffer() {
  if (!in_stream.hasData(2))
    return false;

  size_t length = min(in_stream.avail()/2, availSpace());
  const U8* iptr = in_stream.getptr(length*2);

  U8* optr = (U8*) end;
  for (size_t i=0; i<length; i++) {
    int v = 0;
    readHexAndShift(iptr[i*2], &v);
    readHexAndShift(iptr[i*2+1], &v);
    optr[i] = v;
  }

  in_stream.setptr(length*2);
  end += length;

  return true;
}
