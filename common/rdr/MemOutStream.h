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
// A MemOutStream grows as needed when data is written to it.
//

#ifndef __RDR_MEMOUTSTREAM_H__
#define __RDR_MEMOUTSTREAM_H__

#include <rdr/Exception.h>
#include <rdr/OutStream.h>

namespace rdr {

  class MemOutStream : public OutStream {

  public:

    MemOutStream(int len=1024) {
      start = ptr = new U8[len];
      end = start + len;
    }

    virtual ~MemOutStream() {
      delete [] start;
    }

    void writeBytes(const void* data, size_t length) {
      check(length);
      memcpy(ptr, data, length);
      ptr += length;
    }

    size_t length() { return ptr - start; }
    void clear() { ptr = start; };
    void clearAndZero() { memset(start, 0, ptr-start); clear(); }
    void reposition(size_t pos) { ptr = start + pos; }

    // data() returns a pointer to the buffer.

    const void* data() { return (const void*)start; }

  protected:

    // overrun() either doubles the buffer or adds enough space for nItems of
    // size itemSize bytes.

    size_t overrun(size_t itemSize, size_t nItems) {
      size_t len = ptr - start + itemSize * nItems;
      if (len < (size_t)(end - start) * 2)
        len = (end - start) * 2;

      if (len < (size_t)(end - start))
        throw Exception("Overflow in MemOutStream::overrun()");

      U8* newStart = new U8[len];
      memcpy(newStart, start, ptr - start);
      ptr = newStart + (ptr - start);
      delete [] start;
      start = newStart;
      end = newStart + len;

      return nItems;
    }

    U8* start;
  };

}

#endif
