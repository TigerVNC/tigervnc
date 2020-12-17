/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
// rdr::OutStream marshalls data into a buffer stored in RDR (RFB Data
// Representation).
//

#ifndef __RDR_OUTSTREAM_H__
#define __RDR_OUTSTREAM_H__

#include <rdr/types.h>
#include <rdr/InStream.h>
#include <string.h> // for memcpy

namespace rdr {

  class OutStream {

  protected:

    OutStream() {}

  public:

    virtual ~OutStream() {}

    // check() ensures there is buffer space for at least one item of size
    // itemSize bytes.  Returns the number of items which fit (up to a maximum
    // of nItems).

    inline size_t check(size_t itemSize, size_t nItems=1)
    {
      size_t nAvail;

      if (itemSize > (size_t)(end - ptr))
        return overrun(itemSize, nItems);

      nAvail = (end - ptr) / itemSize;
      if (nAvail < nItems)
        return nAvail;

      return nItems;
    }

    // writeU/SN() methods write unsigned and signed N-bit integers.

    inline void writeU8( U8  u) { check(1); *ptr++ = u; }
    inline void writeU16(U16 u) { check(2); *ptr++ = u >> 8; *ptr++ = (U8)u; }
    inline void writeU32(U32 u) { check(4); *ptr++ = u >> 24; *ptr++ = u >> 16;
                                            *ptr++ = u >> 8; *ptr++ = u; }

    inline void writeS8( S8  s) { writeU8((U8)s); }
    inline void writeS16(S16 s) { writeU16((U16)s); }
    inline void writeS32(S32 s) { writeU32((U32)s); }

    // writeString() writes a string - a U32 length followed by the data.  The
    // given string should be null-terminated (but the terminating null is not
    // written to the stream).

    inline void writeString(const char* str) {
      U32 len = strlen(str);
      writeU32(len);
      writeBytes(str, len);
    }

    inline void pad(size_t bytes) {
      while (bytes-- > 0) writeU8(0);
    }

    inline void skip(size_t bytes) {
      while (bytes > 0) {
        size_t n = check(1, bytes);
        ptr += n;
        bytes -= n;
      }
    }

    // writeBytes() writes an exact number of bytes.

    void writeBytes(const void* data, size_t length) {
      while (length > 0) {
        size_t n = check(1, length);
        memcpy(ptr, data, n);
        ptr += n;
        data = (U8*)data + n;
        length -= n;
      }
    }

    // copyBytes() efficiently transfers data between streams

    void copyBytes(InStream* is, size_t length) {
      while (length > 0) {
        size_t n = check(1, length);
        is->readBytes(ptr, n);
        ptr += n;
        length -= n;
      }
    }

    // writeOpaqueN() writes a quantity without byte-swapping.

    inline void writeOpaque8( U8  u) { writeU8(u); }
    inline void writeOpaque16(U16 u) { check(2); *ptr++ = ((U8*)&u)[0];
                                       *ptr++ = ((U8*)&u)[1]; }
    inline void writeOpaque32(U32 u) { check(4); *ptr++ = ((U8*)&u)[0];
                                       *ptr++ = ((U8*)&u)[1];
                                       *ptr++ = ((U8*)&u)[2];
                                       *ptr++ = ((U8*)&u)[3]; }

    // length() returns the length of the stream.

    virtual size_t length() = 0;

    // flush() requests that the stream be flushed.

    virtual void flush() {}

    // getptr(), getend() and setptr() are "dirty" methods which allow you to
    // manipulate the buffer directly.  This is useful for a stream which is a
    // wrapper around an underlying stream.

    inline U8* getptr() { return ptr; }
    inline U8* getend() { return end; }
    inline void setptr(U8* p) { ptr = p; }

  private:

    // overrun() is implemented by a derived class to cope with buffer overrun.
    // It ensures there are at least itemSize bytes of buffer space.  Returns
    // the number of items which fit (up to a maximum of nItems).  itemSize is
    // supposed to be "small" (a few bytes).

    virtual size_t overrun(size_t itemSize, size_t nItems) = 0;

  protected:

    U8* ptr;
    U8* end;
  };

}

#endif
