/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Martin Koegler
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rdr/Exception.h>
#include <rdr/TLSException.h>
#include <rdr/TLSOutStream.h>
#include <errno.h>

#ifdef HAVE_GNUTLS
using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };

ssize_t TLSOutStream::push(gnutls_transport_ptr_t str, const void* data,
				   size_t size)
{
  TLSOutStream* self= (TLSOutStream*) str;
  OutStream *out = self->out;

  try {
    out->writeBytes(data, size);
    out->flush();
  } catch (Exception& e) {
    gnutls_transport_set_errno(self->session, EINVAL);
    return -1;
  }

  return size;
}

TLSOutStream::TLSOutStream(OutStream* _out, gnutls_session_t _session)
  : session(_session), out(_out), bufSize(DEFAULT_BUF_SIZE), offset(0)
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
}

size_t TLSOutStream::length()
{
  return offset + ptr - start;
}

void TLSOutStream::flush()
{
  U8* sentUpTo = start;
  while (sentUpTo < ptr) {
    size_t n = writeTLS(sentUpTo, ptr - sentUpTo);
    sentUpTo += n;
    offset += n;
  }

  ptr = start;
  out->flush();
}

size_t TLSOutStream::overrun(size_t itemSize, size_t nItems)
{
  if (itemSize > bufSize)
    throw Exception("TLSOutStream overrun: max itemSize exceeded");

  flush();

  size_t nAvail;
  nAvail = (end - ptr) / itemSize;
  if (nAvail < nItems)
    return nAvail;

  return nItems;
}

size_t TLSOutStream::writeTLS(const U8* data, size_t length)
{
  int n;

  n = gnutls_record_send(session, data, length);
  if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN)
    return 0;

  if (n < 0)
    throw TLSException("writeTLS", n);

  return n;
}

#endif
