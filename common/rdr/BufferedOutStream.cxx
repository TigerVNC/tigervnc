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
static const size_t MAX_BUF_SIZE = 32 * 1024 * 1024;

BufferedOutStream::BufferedOutStream(bool emulateCork)
  : bufSize(DEFAULT_BUF_SIZE), offset(0), emulateCork(emulateCork)
{
  ptr = start = sentUpTo = new U8[bufSize];
  end = start + bufSize;
  gettimeofday(&lastSizeCheck, NULL);
  peakUsage = 0;
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

void BufferedOutStream::flush()
{
  struct timeval now;

  // Only give larger chunks if corked to minimize overhead
  if (corked && emulateCork && ((ptr - sentUpTo) < 1024))
    return;

  while (sentUpTo < ptr) {
    size_t len;

    len = (ptr - sentUpTo);

    if (!flushBuffer())
      break;

    offset += len - (ptr - sentUpTo);
  }

  // Managed to flush everything?
  if (sentUpTo == ptr)
    ptr = sentUpTo = start;

  // Time to shrink an excessive buffer?
  gettimeofday(&now, NULL);
  if ((sentUpTo == ptr) && (bufSize > DEFAULT_BUF_SIZE) &&
      ((now.tv_sec < lastSizeCheck.tv_sec) ||
       (now.tv_sec > (lastSizeCheck.tv_sec + 5)))) {
    if (peakUsage < (bufSize / 2)) {
      size_t newSize;

      newSize = DEFAULT_BUF_SIZE;
      while (newSize < peakUsage)
        newSize *= 2;

      // We know the buffer is empty, so just reset everything
      delete [] start;
      ptr = start = sentUpTo = new U8[newSize];
      end = start + newSize;
      bufSize = newSize;
    }

    gettimeofday(&lastSizeCheck, NULL);
    peakUsage = 0;
  }
}

bool BufferedOutStream::hasBufferedData()
{
  return sentUpTo != ptr;
}

void BufferedOutStream::overrun(size_t needed)
{
  bool oldCorked;
  size_t totalNeeded, newSize;
  U8* newBuffer;

  // First try to get rid of the data we have
  // (use corked to make things a bit more efficient since we're not
  // trying to flush out everything, just make some room)
  oldCorked = corked;
  cork(true);
  flush();
  cork(oldCorked);

  // Make note of the total needed space
  totalNeeded = needed + (ptr - sentUpTo);

  if (totalNeeded > peakUsage)
    peakUsage = totalNeeded;

  // Enough free space now?
  if (avail() > needed)
    return;

  // Can we shuffle things around?
  if (needed < bufSize - (ptr - sentUpTo)) {
    memmove(start, sentUpTo, ptr - sentUpTo);
    ptr = start + (ptr - sentUpTo);
    sentUpTo = start;
    return;
  }

  // We'll need to allocate more buffer space...

  if (totalNeeded > MAX_BUF_SIZE)
    throw Exception("BufferedOutStream overrun: requested size of "
                    "%lu bytes exceeds maximum of %lu bytes",
                    (long unsigned)totalNeeded,
                    (long unsigned)MAX_BUF_SIZE);

  newSize = DEFAULT_BUF_SIZE;
  while (newSize < totalNeeded)
    newSize *= 2;

  newBuffer = new U8[newSize];
  memcpy(newBuffer, sentUpTo, ptr - sentUpTo);
  delete [] start;
  bufSize = newSize;

  ptr = newBuffer + (ptr - sentUpTo);
  sentUpTo = start = newBuffer;
  end = newBuffer + newSize;

  gettimeofday(&lastSizeCheck, NULL);
  peakUsage = totalNeeded;

  return;
}
