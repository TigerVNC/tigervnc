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
#include <rdr/TLSOutStream.h>
#include <rfb/LogWriter.h>
#include <errno.h>

#ifdef HAVE_GNUTLS
using namespace rdr;

static rfb::LogWriter vlog("TLSOutStream");

enum { DEFAULT_BUF_SIZE = 16384 };

ssize_t TLSOutStream::push(gnutls_transport_ptr_t str, const void* data,
				   size_t size)
{
  TLSOutStream* self= (TLSOutStream*) str;
  OutStream *out = self->out;

  delete self->saved_exception;
  self->saved_exception = NULL;

  try {
    out->writeBytes(data, size);
    out->flush();
  } catch (SystemException &e) {
    vlog.error("Failure sending TLS data: %s", e.str());
    gnutls_transport_set_errno(self->session, e.err);
    self->saved_exception = new SystemException(e);
    return -1;
  } catch (Exception& e) {
    vlog.error("Failure sending TLS data: %s", e.str());
    gnutls_transport_set_errno(self->session, EINVAL);
    self->saved_exception = new Exception(e);
    return -1;
  }

  return size;
}

TLSOutStream::TLSOutStream(OutStream* _out, gnutls_session_t _session)
  : session(_session), out(_out), bufSize(DEFAULT_BUF_SIZE), offset(0),
    saved_exception(NULL)
{
  gnutls_transport_ptr_t recv, send;

  ptr = start = new U8[bufSize];
  end = start + bufSize;

  gnutls_transport_set_push_function(session, push);
  gnutls_transport_get_ptr2(session, &recv, &send);
  gnutls_transport_set_ptr2(session, recv, this);
}

TLSOutStream::~TLSOutStream()
{
#if 0
  try {
//    flush();
  } catch (Exception&) {
  }
#endif
  gnutls_transport_set_push_function(session, NULL);

  delete [] start;
  delete saved_exception;
}

size_t TLSOutStream::length()
{
  return offset + ptr - start;
}

void TLSOutStream::flush()
{
  U8* sentUpTo;

  // Only give GnuTLS larger chunks if corked to minimize overhead
  if (corked && ((ptr - start) < 1024))
    return;

  sentUpTo = start;
  while (sentUpTo < ptr) {
    size_t n = writeTLS(sentUpTo, ptr - sentUpTo);
    sentUpTo += n;
    offset += n;
  }

  ptr = start;
  out->flush();
}

void TLSOutStream::cork(bool enable)
{
  OutStream::cork(enable);

  out->cork(enable);
}

void TLSOutStream::overrun(size_t needed)
{
  if (needed > bufSize)
    throw Exception("TLSOutStream overrun: buffer size exceeded");

  // A cork might prevent the flush, so disable it temporarily
  corked = false;
  flush();
  corked = true;
}

size_t TLSOutStream::writeTLS(const U8* data, size_t length)
{
  int n;

  n = gnutls_record_send(session, data, length);
  if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN)
    return 0;

  if (n == GNUTLS_E_PUSH_ERROR)
    throw *saved_exception;

  if (n < 0)
    throw TLSException("writeTLS", n);

  return n;
}

#endif
