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

#include <rdr/Exception.h>
#include <rdr/TLSException.h>
#include <rdr/TLSInStream.h>
#include <rfb/LogWriter.h>
#include <errno.h>

#ifdef HAVE_GNUTLS 
using namespace rdr;

static rfb::LogWriter vlog("TLSInStream");

ssize_t TLSInStream::pull(gnutls_transport_ptr_t str, void* data, size_t size)
{
  TLSInStream* self= (TLSInStream*) str;
  InStream *in = self->in;

  self->streamEmpty = false;
  delete self->saved_exception;
  self->saved_exception = NULL;

  try {
    if (!in->hasData(1)) {
      self->streamEmpty = true;
      gnutls_transport_set_errno(self->session, EAGAIN);
      return -1;
    }

    if (in->avail() < size)
      size = in->avail();
  
    in->readBytes(data, size);
  } catch (EndOfStream&) {
    return 0;
  } catch (SystemException &e) {
    vlog.error("Failure reading TLS data: %s", e.str());
    gnutls_transport_set_errno(self->session, e.err);
    self->saved_exception = new SystemException(e);
    return -1;
  } catch (Exception& e) {
    vlog.error("Failure reading TLS data: %s", e.str());
    gnutls_transport_set_errno(self->session, EINVAL);
    self->saved_exception = new Exception(e);
    return -1;
  }

  return size;
}

TLSInStream::TLSInStream(InStream* _in, gnutls_session_t _session)
  : session(_session), in(_in), saved_exception(NULL)
{
  gnutls_transport_ptr_t recv, send;

  gnutls_transport_set_pull_function(session, pull);
  gnutls_transport_get_ptr2(session, &recv, &send);
  gnutls_transport_set_ptr2(session, this, send);
}

TLSInStream::~TLSInStream()
{
  gnutls_transport_set_pull_function(session, NULL);

  delete saved_exception;
}

bool TLSInStream::fillBuffer()
{
  size_t n = readTLS((U8*) end, availSpace());
  if (n == 0)
    return false;
  end += n;

  return true;
}

size_t TLSInStream::readTLS(U8* buf, size_t len)
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
    throw *saved_exception;

  if (n < 0)
    throw TLSException("readTLS", n);

  if (n == 0)
    throw EndOfStream();

  return n;
}

#endif
