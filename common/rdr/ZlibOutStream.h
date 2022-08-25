/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
// ZlibOutStream streams to a compressed data stream (underlying), compressing
// with zlib on the fly.
//

#ifndef __RDR_ZLIBOUTSTREAM_H__
#define __RDR_ZLIBOUTSTREAM_H__

#include <rdr/BufferedOutStream.h>

struct z_stream_s;

namespace rdr {

  class ZlibOutStream : public BufferedOutStream {

  public:

    ZlibOutStream(OutStream* os=0, int compressionLevel=-1);
    virtual ~ZlibOutStream();

    void setUnderlying(OutStream* os);
    void setCompressionLevel(int level=-1);
    virtual void flush();
    virtual void cork(bool enable);

  private:
    virtual bool flushBuffer();
    void deflate(int flush);
    void checkCompressionLevel();

    OutStream* underlying;
    int compressionLevel;
    int newLevel;
    z_stream_s* zs;
  };

} // end of namespace rdr

#endif
