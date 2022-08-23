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
// FdInStream streams from a file descriptor.
//

#ifndef __RDR_FDINSTREAM_H__
#define __RDR_FDINSTREAM_H__

#include <rdr/BufferedInStream.h>

namespace rdr {

  class FdInStream : public BufferedInStream {

  public:

    FdInStream(int fd, bool closeWhenDone_=false);
    virtual ~FdInStream();

    int getFd() { return fd; }

  private:
    virtual bool fillBuffer();

    size_t readFd(void* buf, size_t len);

    int fd;
    bool closeWhenDone;
  };

} // end of namespace rdr

#endif
