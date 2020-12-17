/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <assert.h>

#include <rdr/ZlibInStream.h>
#include <rdr/Exception.h>
#include <zlib.h>

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };

ZlibInStream::ZlibInStream(size_t bufSize_)
  : underlying(0), bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0),
    zs(NULL), bytesIn(0)
{
  ptr = end = start = new U8[bufSize];
  init();
}

ZlibInStream::~ZlibInStream()
{
  deinit();
  delete [] start;
}

void ZlibInStream::setUnderlying(InStream* is, size_t bytesIn_)
{
  underlying = is;
  bytesIn = bytesIn_;
  ptr = end = start;
}

size_t ZlibInStream::pos()
{
  return offset + ptr - start;
}

void ZlibInStream::flushUnderlying()
{
  ptr = end = start;

  while (bytesIn > 0) {
    decompress(true);
    end = start; // throw away any data
  }

  setUnderlying(NULL, 0);
}

void ZlibInStream::reset()
{
  deinit();
  init();
}

void ZlibInStream::init()
{
  assert(zs == NULL);

  zs = new z_stream;
  zs->zalloc    = Z_NULL;
  zs->zfree     = Z_NULL;
  zs->opaque    = Z_NULL;
  zs->next_in   = Z_NULL;
  zs->avail_in  = 0;
  if (inflateInit(zs) != Z_OK) {
    delete zs;
    zs = NULL;
    throw Exception("ZlibInStream: inflateInit failed");
  }
}

void ZlibInStream::deinit()
{
  assert(zs != NULL);
  setUnderlying(NULL, 0);
  inflateEnd(zs);
  delete zs;
  zs = NULL;
}

size_t ZlibInStream::overrun(size_t itemSize, size_t nItems, bool wait)
{
  if (itemSize > bufSize)
    throw Exception("ZlibInStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  offset += ptr - start;
  end -= ptr - start;
  ptr = start;

  while ((size_t)(end - ptr) < itemSize) {
    if (!decompress(wait))
      return 0;
  }

  size_t nAvail;
  nAvail = (end - ptr) / itemSize;
  if (nAvail < nItems)
    return nAvail;

  return nItems;
}

// decompress() calls the decompressor once.  Note that this won't necessarily
// generate any output data - it may just consume some input data.  Returns
// false if wait is false and we would block on the underlying stream.

bool ZlibInStream::decompress(bool wait)
{
  if (!underlying)
    throw Exception("ZlibInStream overrun: no underlying stream");

  zs->next_out = (U8*)end;
  zs->avail_out = start + bufSize - end;

  size_t n = underlying->check(1, 1, wait);
  if (n == 0) return false;
  zs->next_in = (U8*)underlying->getptr();
  zs->avail_in = underlying->getend() - underlying->getptr();
  if (zs->avail_in > bytesIn)
    zs->avail_in = bytesIn;

  int rc = inflate(zs, Z_SYNC_FLUSH);
  if (rc < 0) {
    throw Exception("ZlibInStream: inflate failed");
  }

  bytesIn -= zs->next_in - underlying->getptr();
  end = zs->next_out;
  underlying->setptr(zs->next_in);
  return true;
}
