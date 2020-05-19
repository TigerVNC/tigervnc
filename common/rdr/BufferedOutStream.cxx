/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2020 Pierre Ossman for Cendio AB
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
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

#include <rdr/BufferedOutStream.h>
#include <rdr/Exception.h>


using namespace rdr;

static const size_t DEFAULT_BUF_SIZE = 16384;

BufferedOutStream::BufferedOutStream()
  : bufSize(DEFAULT_BUF_SIZE), offset(0)
{
  ptr = start = sentUpTo = new U8[bufSize];
  end = start + bufSize;
}

BufferedOutStream::~BufferedOutStream()
{
  // FIXME: Complain about non-flushed buffer?
  delete [] start;
}

size_t BufferedOutStream::length()
{
  return offset + ptr - sentUpTo;
}

size_t BufferedOutStream::bufferUsage()
{
  return ptr - sentUpTo;
}

void BufferedOutStream::flush()
{
  while (sentUpTo < ptr) {
    size_t len;

    len = bufferUsage();

    if (!flushBuffer(false))
      break;

    offset += len - bufferUsage();
  }

  // Managed to flush everything?
  if (sentUpTo == ptr)
    ptr = sentUpTo = start;
}

void BufferedOutStream::overrun(size_t needed)
{
  if (needed > bufSize)
    throw Exception("BufferedOutStream overrun: "
                    "requested size of %lu bytes exceeds maximum of %lu bytes",
                    (long unsigned)needed, (long unsigned)bufSize);

  // First try to get rid of the data we have
  flush();

  // Still not enough space?
  while (needed > avail()) {
    // Can we shuffle things around?
    // (don't do this if it gains us less than 25%)
    if (((size_t)(sentUpTo - start) > bufSize / 4) &&
        (needed < bufSize - (ptr - sentUpTo))) {
      memmove(start, sentUpTo, ptr - sentUpTo);
      ptr = start + (ptr - sentUpTo);
      sentUpTo = start;
    } else {
      size_t len;

      len = bufferUsage();

      // Have to get rid of more data, so allow the flush to wait...
      flushBuffer(true);

      offset += len - bufferUsage();

       // Managed to flush everything?
      if (sentUpTo == ptr)
        ptr = sentUpTo = start;
    }
  }
}
