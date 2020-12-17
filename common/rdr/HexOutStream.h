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

#ifndef __RDR_HEX_OUTSTREAM_H__
#define __RDR_HEX_OUTSTREAM_H__

#include <rdr/OutStream.h>

namespace rdr {

  class HexOutStream : public OutStream {
  public:

    HexOutStream(OutStream& os, size_t buflen=0);
    virtual ~HexOutStream();

    void flush();
    size_t length();

    static char intToHex(int i);
    static char* binToHexStr(const char* data, size_t length);

  private:
    void writeBuffer();
    size_t overrun(size_t itemSize, size_t nItems);

    OutStream& out_stream;

    U8* start;
    size_t offset;
    size_t bufSize;
  };

}

#endif
