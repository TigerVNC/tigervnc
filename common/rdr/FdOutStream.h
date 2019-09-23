/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman for Cendio AB
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
// FdOutStream streams to a file descriptor.
//

#ifndef __RDR_FDOUTSTREAM_H__
#define __RDR_FDOUTSTREAM_H__

#include <sys/time.h>

#include <rdr/OutStream.h>

namespace rdr {

  class FdOutStream : public OutStream {

  public:

    FdOutStream(int fd, bool blocking=true, int timeoutms=-1, size_t bufSize=0);
    virtual ~FdOutStream();

    void setTimeout(int timeoutms);
    void setBlocking(bool blocking);
    int getFd() { return fd; }

    void flush();
    size_t length();

    int bufferUsage();

    unsigned getIdleTime();

  private:
    size_t overrun(size_t itemSize, size_t nItems);
    size_t writeWithTimeout(const void* data, size_t length, int timeoutms);
    int fd;
    bool blocking;
    int timeoutms;
    size_t bufSize;
    size_t offset;
    U8* start;
    U8* sentUpTo;
    struct timeval lastWrite;
  };

}

#endif
