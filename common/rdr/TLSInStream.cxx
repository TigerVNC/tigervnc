/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2012-2021 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rdr/TLSException.h>
#include <rdr/TLSInStream.h>

#include <errno.h>

#ifdef HAVE_GNUTLS 
using namespace rdr;

static core::LogWriter vlog("TLSInStream");

ssize_t TLSInStream::pull(gnutls_transport_ptr_t str, void* data, size_t size)
{
  TLSInStream* self= (TLSInStream*) str;
  InStream *in = self->in;

  self->streamEmpty = false;
  self->saved_exception = nullptr;

  try {
    if (!in->hasData(1)) {
      self->streamEmpty = true;
      gnutls_transport_set_errno(self->session, EAGAIN);
      return -1;
    }

    if (in->avail() < size)
      size = in->avail();
  
    in->readBytes((uint8_t*)data, size);
  } catch (end_of_stream&) {
    return 0;
  } catch (std::exception& e) {
    core::socket_error* se;
    vlog.error("Failure reading TLS data: %s", e.what());
    se = dynamic_cast<core::socket_error*>(&e);
    if (se)
      gnutls_transport_set_errno(self->session, se->err);
    else
      gnutls_transport_set_errno(self->session, EINVAL);
    self->saved_exception = std::current_exception();
    return -1;
  }

  return size;
}

TLSInStream::TLSInStream(InStream* _in, gnutls_session_t _session)
  : session(_session), in(_in)
{
  gnutls_transport_ptr_t recv, send;

  gnutls_transport_set_pull_function(session, pull);
  gnutls_transport_get_ptr2(session, &recv, &send);
  gnutls_transport_set_ptr2(session, this, send);
}

TLSInStream::~TLSInStream()
{
  gnutls_transport_set_pull_function(session, nullptr);
}

bool TLSInStream::fillBuffer()
{
  size_t n = readTLS((uint8_t*) end, availSpace());
  if (n == 0)
    return false;
  end += n;

  return true;
}

size_t TLSInStream::readTLS(uint8_t* buf, size_t len)
{
  int n;

  while (true) {
    streamEmpty = false;
    n = gnutls_record_recv(session, (void *) buf, len);
    if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN) {
      // GnuTLS returns GNUTLS_E_AGAIN for a bunch of other scenarios
      // other than the pull function returning EAGAIN, so we have to
      // double check that the underlying stream really is empty
      if (!streamEmpty)
        continue;
      else
        return 0;
    }
    break;
  };

  if (n == GNUTLS_E_PULL_ERROR)
    std::rethrow_exception(saved_exception);

  if (n < 0)
    throw tls_error("readTLS", n);

  if (n == 0)
    throw end_of_stream();

  return n;
}

#endif
