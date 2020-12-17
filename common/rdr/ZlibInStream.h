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
// ZlibInStream streams from a compressed data stream ("underlying"),
// decompressing with zlib on the fly.
//

#ifndef __RDR_ZLIBINSTREAM_H__
#define __RDR_ZLIBINSTREAM_H__

#include <rdr/InStream.h>

struct z_stream_s;

namespace rdr {

  class ZlibInStream : public InStream {

  public:

    ZlibInStream(size_t bufSize=0);
    virtual ~ZlibInStream();

    void setUnderlying(InStream* is, size_t bytesIn);
    void flushUnderlying();
    size_t pos();
    void reset();

  private:

    void init();
    void deinit();

    size_t overrun(size_t itemSize, size_t nItems, bool wait);
    bool decompress(bool wait);

    InStream* underlying;
    size_t bufSize;
    size_t offset;
    z_stream_s* zs;
    size_t bytesIn;
    U8* start;
  };

} // end of namespace rdr

#endif
