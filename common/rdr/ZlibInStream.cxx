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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <rdr/ZlibInStream.h>
#include <rdr/Exception.h>
#include <zlib.h>

using namespace rdr;

ZlibInStream::ZlibInStream()
  : underlying(0), zs(NULL), bytesIn(0)
{
  init();
}

ZlibInStream::~ZlibInStream()
{
  deinit();
}

void ZlibInStream::setUnderlying(InStream* is, size_t bytesIn_)
{
  underlying = is;
  bytesIn = bytesIn_;
  skip(avail());
}

void ZlibInStream::flushUnderlying()
{
  while (bytesIn > 0) {
    if (!hasData(1))
      throw Exception("ZlibInStream: failed to flush remaining stream data");
    skip(avail());
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

bool ZlibInStream::fillBuffer()
{
  if (!underlying)
    throw Exception("ZlibInStream overrun: no underlying stream");

  zs->next_out = (U8*)end;
  zs->avail_out = availSpace();

  if (!underlying->hasData(1))
    return false;
  size_t length = underlying->avail();
  if (length > bytesIn)
    length = bytesIn;
  zs->next_in = (U8*)underlying->getptr(length);
  zs->avail_in = length;

  int rc = inflate(zs, Z_SYNC_FLUSH);
  if (rc < 0) {
    throw Exception("ZlibInStream: inflate failed");
  }

  bytesIn -= length - zs->avail_in;
  end = zs->next_out;
  underlying->setptr(length - zs->avail_in);
  return true;
}
