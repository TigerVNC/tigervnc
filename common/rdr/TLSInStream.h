/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
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

#ifndef __RDR_TLSINSTREAM_H__
#define __RDR_TLSINSTREAM_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GNUTLS

#include <gnutls/gnutls.h>
#include <rdr/InStream.h>

namespace rdr {

  class TLSInStream : public InStream {
  public:
    TLSInStream(InStream* in, gnutls_session_t session);
    virtual ~TLSInStream();

    size_t pos();

  private:
    size_t overrun(size_t itemSize, size_t nItems, bool wait);
    size_t readTLS(U8* buf, size_t len, bool wait);
    static ssize_t pull(gnutls_transport_ptr_t str, void* data, size_t size);

    gnutls_session_t session;
    InStream* in;
    size_t bufSize;
    size_t offset;
    U8* start;
  };
};

#endif
#endif
