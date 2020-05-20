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

#include <rdr/HexOutStream.h>
#include <rdr/Exception.h>

using namespace rdr;

const int DEFAULT_BUF_LEN = 16384;

static inline size_t min(size_t a, size_t b) {return a<b ? a : b;}

HexOutStream::HexOutStream(OutStream& os)
  : out_stream(os), offset(0), bufSize(DEFAULT_BUF_LEN)
{
  if (bufSize % 2)
    bufSize--;
  ptr = start = new U8[bufSize];
  end = start + bufSize;
}

HexOutStream::~HexOutStream() {
  delete [] start;
}

char HexOutStream::intToHex(int i) {
  if ((i>=0) && (i<=9))
    return '0'+i;
  else if ((i>=10) && (i<=15))
    return 'a'+(i-10);
  else
    throw rdr::Exception("intToHex failed");
}

char* HexOutStream::binToHexStr(const char* data, size_t length) {
  char* buffer = new char[length*2+1];
  for (size_t i=0; i<length; i++) {
    buffer[i*2] = intToHex((data[i] >> 4) & 15);
    buffer[i*2+1] = intToHex((data[i] & 15));
    if (!buffer[i*2] || !buffer[i*2+1]) {
      delete [] buffer;
      return 0;
    }
  }
  buffer[length*2] = 0;
  return buffer;
}


void
HexOutStream::writeBuffer() {
  U8* pos = start;
  while (pos != ptr) {
    U8* optr = out_stream.getptr(2);
    size_t length = min(ptr-pos, out_stream.avail()/2);

    for (size_t i=0; i<length; i++) {
      optr[i*2] = intToHex((pos[i] >> 4) & 0xf);
      optr[i*2+1] = intToHex(pos[i] & 0xf);
    }

    out_stream.setptr(length*2);
    pos += length;
  }
  offset += ptr - start;
  ptr = start;
}

size_t HexOutStream::length()
{
  return offset + ptr - start;
}

void
HexOutStream::flush() {
  writeBuffer();
  out_stream.flush();
}

void HexOutStream::cork(bool enable)
{
  OutStream::cork(enable);

  out_stream.cork(enable);
}

void HexOutStream::overrun(size_t needed) {
  if (needed > bufSize)
    throw Exception("HexOutStream overrun: buffer size exceeded");

  writeBuffer();
}

