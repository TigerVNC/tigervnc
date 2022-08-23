/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2020 Pierre Ossman for Cendio AB
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
// Base class for input streams with a buffer
//

#ifndef __RDR_BUFFEREDINSTREAM_H__
#define __RDR_BUFFEREDINSTREAM_H__

#include <sys/time.h>

#include <rdr/InStream.h>

namespace rdr {

  class BufferedInStream : public InStream {

  public:
    virtual ~BufferedInStream();

    virtual size_t pos();

  protected:
    size_t availSpace() { return start + bufSize - end; }

    void ensureSpace(size_t needed);

  private:
    virtual bool fillBuffer() = 0;

    virtual bool overrun(size_t needed);

  private:
    size_t bufSize;
    size_t offset;
    U8* start;

    struct timeval lastSizeCheck;
    size_t peakUsage;

  protected:
    BufferedInStream();
  };

} // end of namespace rdr

#endif
