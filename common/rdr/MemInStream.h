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
// rdr::MemInStream is an InStream which streams from a given memory buffer.
// If the deleteWhenDone parameter is true then the buffer will be delete[]d in
// the destructor.  Note that it is delete[]d as a uint8_t* - strictly speaking this
// means it ought to be new[]ed as a uint8_t* as well, but on most platforms this
// doesn't matter.
//

#ifndef __RDR_MEMINSTREAM_H__
#define __RDR_MEMINSTREAM_H__

#include <rdr/InStream.h>

namespace rdr {

  class MemInStream : public InStream {

  public:

    MemInStream(const uint8_t* data, size_t len, bool deleteWhenDone_=false)
      : start(data), deleteWhenDone(deleteWhenDone_)
    {
      ptr = start;
      end = start + len;

#ifdef RFB_INSTREAM_CHECK
      // MemInStream cannot add more data, so callers are assumed to already
      // new the total size
      avail();
#endif
    }

    virtual ~MemInStream() {
      if (deleteWhenDone)
        delete [] start;
    }

    size_t pos() override { return ptr - start; }
    void reposition(size_t pos) { ptr = start + pos; }

  private:

    bool overrun(size_t /*needed*/) override { throw end_of_stream(); }
    const uint8_t* start;
    bool deleteWhenDone;
  };

}

#endif
