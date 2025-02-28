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
#include <rdr/TLSOutStream.h>

#include <errno.h>

#ifdef HAVE_GNUTLS
using namespace rdr;

static core::LogWriter vlog("TLSOutStream");

ssize_t TLSOutStream::push(gnutls_transport_ptr_t str, const void* data,
				   size_t size)
{
  TLSOutStream* self= (TLSOutStream*) str;
  OutStream *out = self->out;

  delete self->saved_exception;
  self->saved_exception = nullptr;

  try {
    out->writeBytes((const uint8_t*)data, size);
    out->flush();
  } catch (core::socket_error& e) {
    vlog.error("Failure sending TLS data: %s", e.what());
    gnutls_transport_set_errno(self->session, e.err);
    self->saved_exception = new core::socket_error(e);
    return -1;
  } catch (std::exception& e) {
    vlog.error("Failure sending TLS data: %s", e.what());
    gnutls_transport_set_errno(self->session, EINVAL);
    self->saved_exception = new std::runtime_error(e.what());
    return -1;
  }

  return size;
}

TLSOutStream::TLSOutStream(OutStream* _out, gnutls_session_t _session)
  : session(_session), out(_out), saved_exception(nullptr)
{
  gnutls_transport_ptr_t recv, send;

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
  gnutls_transport_set_push_function(session, nullptr);

  delete saved_exception;
}

void TLSOutStream::flush()
{
  BufferedOutStream::flush();
  out->flush();
}

void TLSOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);
  out->cork(enable);
}

bool TLSOutStream::flushBuffer()
{
  while (sentUpTo < ptr) {
    size_t n = writeTLS(sentUpTo, ptr - sentUpTo);
    sentUpTo += n;
  }

  return true;
}

size_t TLSOutStream::writeTLS(const uint8_t* data, size_t length)
{
  int n;

  n = gnutls_record_send(session, data, length);
  if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN)
    return 0;

  if (n == GNUTLS_E_PUSH_ERROR) {
    if (dynamic_cast<core::socket_error*>(saved_exception))
      throw *dynamic_cast<core::socket_error*>(saved_exception);
    else
      throw std::runtime_error(saved_exception->what());
  }

  if (n < 0)
    throw tls_error("writeTLS", n);

  return n;
}

#endif
