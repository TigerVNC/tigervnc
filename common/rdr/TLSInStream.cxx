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
#include <rdr/TLSInStream.h>
#include <errno.h>

#ifdef HAVE_OLD_GNUTLS
#define gnutls_transport_set_global_errno(A) do { errno = (A); } while(0)
#endif

#ifdef HAVE_GNUTLS 
using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };

ssize_t rdr::gnutls_InStream_pull(gnutls_transport_ptr str, void* data,
				  size_t size)
{
  InStream* in= (InStream*) str;

  try {
    if (!in->check(1, 1, false)) {
      gnutls_transport_set_global_errno(EAGAIN);
      return -1;
    }

    if (in->getend() - in->getptr() < size)
      size = in->getend() - in->getptr();
  
    in->readBytes(data, size);

  } catch (Exception& e) {
    gnutls_transport_set_global_errno(EINVAL);
    return -1;
  }

  return size;
}

TLSInStream::TLSInStream(InStream* _in, gnutls_session _session)
  : session(_session), in(_in), bufSize(DEFAULT_BUF_SIZE), offset(0)
{
  ptr = end = start = new U8[bufSize];
}

TLSInStream::~TLSInStream()
{
  delete[] start;
}

int TLSInStream::pos()
{
  return offset + ptr - start;
}

int TLSInStream::overrun(int itemSize, int nItems, bool wait)
{
  if (itemSize > bufSize)
    throw Exception("TLSInStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  offset += ptr - start;
  end -= ptr - start;
  ptr = start;

  while (end < start + itemSize) {
    int n = readTLS((U8*) end, start + bufSize - end, wait);
    if (!wait && n == 0)
      return 0;
    end += n;
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}

int TLSInStream::readTLS(U8* buf, int len, bool wait)
{
  int n;

  n = in->check(1, 1, wait);
  if (n == 0)
    return 0;

  n = gnutls_record_recv(session, (void *) buf, len);
  if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN)
    return 0;

  if (n < 0) throw TLSException("readTLS", n);

  return n;
}

#endif
