/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Martin Koegler
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rdr/TLSOutStream.h>
#include <rdr/TLSSocket.h>

#ifdef HAVE_GNUTLS

using namespace rdr;

TLSOutStream::TLSOutStream(TLSSocket* sock_)
  : sock(sock_)
{
}

TLSOutStream::~TLSOutStream()
{
}

void TLSOutStream::flush()
{
  BufferedOutStream::flush();
  sock->out->flush();
}

void TLSOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);
  sock->out->cork(enable);
}

bool TLSOutStream::flushBuffer()
{
  while (sentUpTo < ptr) {
    size_t n = sock->writeTLS(sentUpTo, ptr - sentUpTo);
    sentUpTo += n;
  }

  return true;
}

#endif
