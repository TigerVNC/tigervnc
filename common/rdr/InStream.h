/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2020 Pierre Ossman for Cendio AB
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
#include <rdr/Exception.h>
#include <string.h> // for memcpy

// Check that callers are using InStream properly,
// useful when writing new protocol handling
#ifdef _DEBUG
#define RFB_INSTREAM_CHECK
#endif

namespace rdr {

  class InStream {

  public:

    virtual ~InStream() {}

    // avail() returns the number of bytes that are currenctly directly
    // available from the stream.

    inline size_t avail() {
#ifdef RFB_INSTREAM_CHECK
      checkedBytes = end - ptr;
#endif

      return end - ptr;
    }

    // hasData() ensures there is at least "length" bytes of buffer data,
    // possibly trying to fetch more data if there isn't enough right away

    inline bool hasData(size_t length) {
#ifdef RFB_INSTREAM_CHECK
      checkedBytes = 0;
#endif

      if (length > (size_t)(end - ptr)) {
        if (restorePoint != NULL) {
          bool ret;
          size_t restoreDiff;

          restoreDiff = ptr - restorePoint;
          ptr = restorePoint;

          ret = overrun(length + restoreDiff);

          restorePoint = ptr;
          ptr += restoreDiff;

          if (!ret)
            return false;
        } else {
          if (!overrun(length))
            return false;
        }
      }

#ifdef RFB_INSTREAM_CHECK
      checkedBytes = length;
#endif

      return true;
    }

    inline bool hasDataOrRestore(size_t length) {
      if (hasData(length))
        return true;
      gotoRestorePoint();
      return false;
    }

    inline void setRestorePoint() {
#ifdef RFB_INSTREAM_CHECK
      if (restorePoint != NULL)
        throw Exception("Nested use of input stream restore point");
#endif
      restorePoint = ptr;
    }
    inline void clearRestorePoint() {
#ifdef RFB_INSTREAM_CHECK
      if (restorePoint == NULL)
        throw Exception("Incorrect clearing of input stream restore point");
#endif
      restorePoint = NULL;
    }
    inline void gotoRestorePoint() {
#ifdef RFB_INSTREAM_CHECK
      if (restorePoint == NULL)
        throw Exception("Incorrect activation of input stream restore point");
#endif
      ptr = restorePoint;
      clearRestorePoint();
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

    // skip() ignores a number of bytes on the stream

    inline void skip(size_t bytes) {
      check(bytes);
      ptr += bytes;
    }

    // readBytes() reads an exact number of bytes.

    void readBytes(void* data, size_t length) {
      check(length);
      memcpy(data, ptr, length);
      ptr += length;
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

    // getptr() and setptr() are "dirty" methods which allow you direct access
    // to the buffer. This is useful for a stream which is a wrapper around an
    // some other stream API.

    inline const U8* getptr(size_t length) { check(length);
                                             return ptr; }
    inline void setptr(size_t length) { if (length > avail())
                                          throw Exception("Input stream overflow");
                                        skip(length); }

  private:

    const U8* restorePoint;
#ifdef RFB_INSTREAM_CHECK
    size_t checkedBytes;
#endif

    inline void check(size_t bytes) {
#ifdef RFB_INSTREAM_CHECK
      if (bytes > checkedBytes)
        throw Exception("Input stream used without underrun check");
      checkedBytes -= bytes;
#endif
      if (bytes > (size_t)(end - ptr))
        throw Exception("InStream buffer underrun");
    }

    // overrun() is implemented by a derived class to cope with buffer overrun.
    // It tries to ensure there are at least needed bytes of buffer data.
    // Returns true if it managed to satisfy the request, or false otherwise.

    virtual bool overrun(size_t needed) = 0;

  protected:

    InStream() : restorePoint(NULL)
#ifdef RFB_INSTREAM_CHECK
      ,checkedBytes(0)
#endif
     {}
    const U8* ptr;
    const U8* end;
  };

}

#endif
