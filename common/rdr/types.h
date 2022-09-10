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

#ifndef __RDR_TYPES_H__
#define __RDR_TYPES_H__

#include <stdint.h>

namespace rdr {

  class U8Array {
  public:
    U8Array() : buf(0) {}
    U8Array(uint8_t* a) : buf(a) {} // note: assumes ownership
    U8Array(int len) : buf(new uint8_t[len]) {}
    ~U8Array() { delete [] buf; }

    // Get the buffer pointer & clear it (i.e. caller takes ownership)
    uint8_t* takeBuf() { uint8_t* tmp = buf; buf = 0; return tmp; }

    uint8_t* buf;
  };

  class U16Array {
  public:
    U16Array() : buf(0) {}
    U16Array(uint16_t* a) : buf(a) {} // note: assumes ownership
    U16Array(int len) : buf(new uint16_t[len]) {}
    ~U16Array() { delete [] buf; }
    uint16_t* takeBuf() { uint16_t* tmp = buf; buf = 0; return tmp; }
    uint16_t* buf;
  };

  class U32Array {
  public:
    U32Array() : buf(0) {}
    U32Array(uint32_t* a) : buf(a) {} // note: assumes ownership
    U32Array(int len) : buf(new uint32_t[len]) {}
    ~U32Array() { delete [] buf; }
    uint32_t* takeBuf() { uint32_t* tmp = buf; buf = 0; return tmp; }
    uint32_t* buf;
  };

  class S32Array {
  public:
    S32Array() : buf(0) {}
    S32Array(int32_t* a) : buf(a) {} // note: assumes ownership
    S32Array(int len) : buf(new int32_t[len]) {}
    ~S32Array() { delete [] buf; }
    int32_t* takeBuf() { int32_t* tmp = buf; buf = 0; return tmp; }
    int32_t* buf;
  };

} // end of namespace rdr

#endif
