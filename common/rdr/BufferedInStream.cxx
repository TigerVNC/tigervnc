/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2020 Pierre Ossman for Cendio AB
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

#include <assert.h>

#include <rdr/BufferedInStream.h>
#include <rdr/Exception.h>

using namespace rdr;

static const size_t DEFAULT_BUF_SIZE = 8192;
static const size_t MAX_BUF_SIZE = 32 * 1024 * 1024;

BufferedInStream::BufferedInStream()
  : bufSize(DEFAULT_BUF_SIZE), offset(0)
{
  ptr = end = start = new U8[bufSize];
  gettimeofday(&lastSizeCheck, NULL);
  peakUsage = 0;
}

BufferedInStream::~BufferedInStream()
{
  delete [] start;
}

size_t BufferedInStream::pos()
{
  return offset + ptr - start;
}

void BufferedInStream::ensureSpace(size_t needed)
{
  struct timeval now;

  // Given argument is how much free space is needed, but for allocation
  // purposes we need to now how much space everything needs, including
  // any existing data already in the buffer
  needed += avail();

  if (needed > bufSize) {
    size_t newSize;
    U8* newBuffer;

    if (needed > MAX_BUF_SIZE)
      throw Exception("BufferedInStream overrun: requested size of "
                      "%lu bytes exceeds maximum of %lu bytes",
                      (long unsigned)needed, (long unsigned)MAX_BUF_SIZE);

    newSize = DEFAULT_BUF_SIZE;
    while (newSize < needed)
      newSize *= 2;

    newBuffer = new U8[newSize];
    memcpy(newBuffer, ptr, end - ptr);
    delete [] start;
    bufSize = newSize;

    offset += ptr - start;
    end = newBuffer + (end - ptr);
    ptr = start = newBuffer;

    gettimeofday(&lastSizeCheck, NULL);
    peakUsage = needed;
  }

  if (needed > peakUsage)
    peakUsage = needed;

  // Time to shrink an excessive buffer?
  gettimeofday(&now, NULL);
  if ((avail() == 0) && (bufSize > DEFAULT_BUF_SIZE) &&
      ((now.tv_sec < lastSizeCheck.tv_sec) ||
       (now.tv_sec > (lastSizeCheck.tv_sec + 5)))) {
    if (peakUsage < (bufSize / 2)) {
      size_t newSize;

      newSize = DEFAULT_BUF_SIZE;
      while (newSize < peakUsage)
        newSize *= 2;

      // We know the buffer is empty, so just reset everything
      delete [] start;
      ptr = end = start = new U8[newSize];
      bufSize = newSize;
    }

    gettimeofday(&lastSizeCheck, NULL);
    peakUsage = needed;
  }

  // Do we need to shuffle things around?
  if ((bufSize - (ptr - start)) < needed) {
    memmove(start, ptr, end - ptr);

    offset += ptr - start;
    end -= ptr - start;
    ptr = start;
  }
}

bool BufferedInStream::overrun(size_t needed)
{
  // Make sure fillBuffer() has room for all the requested data
  assert(needed > avail());
  ensureSpace(needed - avail());

  while (avail() < needed) {
    if (!fillBuffer())
      return false;
  }

  return true;
}
