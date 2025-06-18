/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright 2012-2025 Pierre Ossman for Cendio AB
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 */

#ifndef __RDR_TLSSOCKET_H__
#define __RDR_TLSSOCKET_H__

#ifdef HAVE_GNUTLS

#include <exception>

#include <gnutls/gnutls.h>

#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>

namespace rdr {

  class InStream;
  class OutStream;

  class TLSInStream;
  class TLSOutStream;

  class TLSSocket {
  public:
    TLSSocket(InStream* in, OutStream* out, gnutls_session_t session);
    virtual ~TLSSocket();

    TLSInStream& inStream() { return tlsin; }
    TLSOutStream& outStream() { return tlsout; }

    bool handshake();
    void shutdown();

  protected:
    /* Used by the stream classes */
    size_t readTLS(uint8_t* buf, size_t len);
    size_t writeTLS(const uint8_t* data, size_t length);

    friend TLSInStream;
    friend TLSOutStream;

  private:
    ssize_t pull(void* data, size_t size);
    ssize_t push(const void* data, size_t size);

    gnutls_session_t session;

    InStream* in;
    OutStream* out;

    TLSInStream tlsin;
    TLSOutStream tlsout;

    bool streamEmpty;

    std::exception_ptr saved_exception;
  };

}

#endif

#endif
