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
// rdr::InStream marshalls data from a buffer stored in RDR (RFB Data
// Representation).
//

#ifndef __RDR_INSTREAM_H__
#define __RDR_INSTREAM_H__

#include <rdr/types.h>
#include <string.h> // for memcpy

namespace rdr {

  class InStream {

  public:

    virtual ~InStream() {}

    // check() ensures there is buffer data for at least one item of size
    // itemSize bytes.  Returns the number of items in the buffer (up to a
    // maximum of nItems).  If wait is false, then instead of blocking to wait
    // for the bytes, zero is returned if the bytes are not immediately
    // available. If itemSize or nItems is zero, check() will return zero.

    inline size_t check(size_t itemSize, size_t nItems=1, bool wait=true)
    {
      size_t nAvail;

      if (itemSize == 0 || nItems == 0)
        return 0;

      if (itemSize > (size_t)(end - ptr))
        return overrun(itemSize, nItems, wait);

      // itemSize cannot be zero at this point
      nAvail = (end - ptr) / itemSize;
      if (nAvail < nItems)
        return nAvail;

      return nItems;
    }

    // checkNoWait() tries to make sure that the given number of bytes can
    // be read without blocking.  It returns true if this is the case, false
    // otherwise.  The length must be "small" (less than the buffer size).
    // If length is zero, checkNoWait() will return true.

    inline bool checkNoWait(size_t length)
    {
      return length == 0 || check(length, 1, false) > 0;
    }

    // readU/SN() methods read unsigned and signed N-bit integers.

    inline U8  readU8()  { check(1); return *ptr++; }
    inline U16 readU16() { check(2); int b0 = *ptr++; int b1 = *ptr++;
                           return b0 << 8 | b1; }
    inline U32 readU32() { check(4); int b0 = *ptr++; int b1 = *ptr++;
                                     int b2 = *ptr++; int b3 = *ptr++;
                           return b0 << 24 | b1 << 16 | b2 << 8 | b3; }

    inline S8  readS8()  { return (S8) readU8();  }
    inline S16 readS16() { return (S16)readU16(); }
    inline S32 readS32() { return (S32)readU32(); }

    // readString() reads a string - a U32 length followed by the data.
    // Returns a null-terminated string - the caller should delete[] it
    // afterwards.

    char* readString();

    // maxStringLength protects against allocating a huge buffer.  Set it
    // higher if you need longer strings.

    static U32 maxStringLength;

    inline void skip(size_t bytes) {
      while (bytes > 0) {
        size_t n = check(1, bytes);
        ptr += n;
        bytes -= n;
      }
    }

    // readBytes() reads an exact number of bytes.
    // If length is zero, readBytes() will return immediately.

    void readBytes(void* data, size_t length) {
      while (length > 0) {
        size_t n = check(1, length);
        memcpy(data, ptr, n);
        ptr += n;
        data = (U8*)data + n;
        length -= n;
      }
    }

    // readOpaqueN() reads a quantity without byte-swapping.

    inline U8  readOpaque8()  { return readU8(); }
    inline U16 readOpaque16() { check(2); U16 r; ((U8*)&r)[0] = *ptr++;
                                ((U8*)&r)[1] = *ptr++; return r; }
    inline U32 readOpaque32() { check(4); U32 r; ((U8*)&r)[0] = *ptr++;
                                ((U8*)&r)[1] = *ptr++; ((U8*)&r)[2] = *ptr++;
                                ((U8*)&r)[3] = *ptr++; return r; }

    // pos() returns the position in the stream.

    virtual size_t pos() = 0;

    // getptr(), getend() and setptr() are "dirty" methods which allow you to
    // manipulate the buffer directly.  This is useful for a stream which is a
    // wrapper around an underlying stream.

    inline const U8* getptr() const { return ptr; }
    inline const U8* getend() const { return end; }
    inline void setptr(const U8* p) { ptr = p; }

  private:

    // overrun() is implemented by a derived class to cope with buffer overrun.
    // It ensures there are at least itemSize bytes of buffer data.  Returns
    // the number of items in the buffer (up to a maximum of nItems).  itemSize
    // is supposed to be "small" (a few bytes).  If wait is false, then
    // instead of blocking to wait for the bytes, zero is returned if the bytes
    // are not immediately available.

    virtual size_t overrun(size_t itemSize, size_t nItems, bool wait=true) = 0;

  protected:

    InStream() {}
    const U8* ptr;
    const U8* end;
  };

}

#endif
