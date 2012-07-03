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

#ifndef __RDR_TLSOUTSTREAM_H__
#define __RDR_TLSOUTSTREAM_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#include <rdr/OutStream.h>

namespace rdr {

  class TLSOutStream : public OutStream {
  public:
    TLSOutStream(OutStream* out, gnutls_session session);
    virtual ~TLSOutStream();

    void flush();
    int length();

  protected:
    int overrun(int itemSize, int nItems);

  private:
    int writeTLS(const U8* data, int length);
    static ssize_t push(gnutls_transport_ptr str, const void* data, size_t size);

    gnutls_session session;
    OutStream* out;
    int bufSize;
    U8* start;
    int offset;
  };
};

#endif
#endif
