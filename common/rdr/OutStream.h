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
#include <rdr/Exception.h>
#include <rdr/InStream.h>
#include <string.h> // for memcpy

namespace rdr {

  class OutStream {

  protected:

    OutStream() : ptr(NULL), end(NULL), corked(false) {}

  public:

    virtual ~OutStream() {}

    // avail() returns the number of bytes that currently be written to the
    // stream without any risk of blocking.

    inline size_t avail()
    {
      return end - ptr;
    }

    // writeU/SN() methods write unsigned and signed N-bit integers.

    inline void writeU8( U8  u) { check(1); *ptr++ = u; }
    inline void writeU16(U16 u) { check(2); *ptr++ = u >> 8; *ptr++ = (U8)u; }
    inline void writeU32(U32 u) { check(4); *ptr++ = u >> 24; *ptr++ = u >> 16;
                                            *ptr++ = u >> 8; *ptr++ = u; }

    inline void writeS8( S8  s) { writeU8((U8)s); }
    inline void writeS16(S16 s) { writeU16((U16)s); }
    inline void writeS32(S32 s) { writeU32((U32)s); }

    inline void pad(size_t bytes) {
      while (bytes-- > 0) writeU8(0);
    }

    // writeBytes() writes an exact number of bytes.

    void writeBytes(const void* data, size_t length) {
      while (length > 0) {
        check(1);
        size_t n = length;
        if (length > avail())
          n = avail();
        memcpy(ptr, data, n);
        ptr += n;
        data = (U8*)data + n;
        length -= n;
      }
    }

    // copyBytes() efficiently transfers data between streams

    void copyBytes(InStream* is, size_t length) {
      while (length > 0) {
        check(1);
        size_t n = length;
        if (length > avail())
          n = avail();
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

    // cork() requests that the stream coalesces flushes in an efficient way

    virtual void cork(bool enable) { corked = enable; if (!enable) flush(); }

    // getptr() and setptr() are "dirty" methods which allow you direct access
    // to the buffer. This is useful for a stream which is a wrapper around an
    // some other stream API. Note that setptr() should not called with a value
    // larger than the bytes actually written as doing so can result in
    // security issues. Use pad() in such cases instead.

    inline U8* getptr(size_t length) { check(length); return ptr; }
    inline void setptr(size_t length) { if (length > avail())
                                          throw Exception("Output stream overflow");
                                        ptr += length; }

  private:

    inline void check(size_t length)
    {
      if (length > avail())
        overrun(length);
    }

    // overrun() is implemented by a derived class to cope with buffer overrun.
    // It ensures there are at least needed bytes of buffer space.

    virtual void overrun(size_t needed) = 0;

  protected:

    U8* ptr;
    U8* end;

    bool corked;
  };

}

#endif
