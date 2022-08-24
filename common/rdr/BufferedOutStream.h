/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2020 Pierre Ossman for Cendio AB
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
// Base class for output streams with a buffer
//

#ifndef __RDR_BUFFEREDOUTSTREAM_H__
#define __RDR_BUFFEREDOUTSTREAM_H__

#include <sys/time.h>

#include <rdr/OutStream.h>

namespace rdr {

  class BufferedOutStream : public OutStream {

  public:
    virtual ~BufferedOutStream();

    virtual size_t length();
    virtual void flush();

    // hasBufferedData() checks if there is any data yet to be flushed

    bool hasBufferedData();

  private:
    // flushBuffer() requests that the stream be flushed. Returns true if it is
    // able to progress the output (which might still not mean any bytes
    // actually moved) and can be called again.

    virtual bool flushBuffer() = 0;

    virtual void overrun(size_t needed);

  private:
    size_t bufSize;
    size_t offset;
    U8* start;

    struct timeval lastSizeCheck;
    size_t peakUsage;

    bool emulateCork;

  protected:
    U8* sentUpTo;

  protected:
    BufferedOutStream(bool emulateCork=true);
  };

}

#endif
