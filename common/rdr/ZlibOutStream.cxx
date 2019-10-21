/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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

#include <stdio.h>

#include <rdr/ZlibOutStream.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>

#include <zlib.h>

#undef ZLIBOUT_DEBUG

static rfb::LogWriter vlog("ZlibOutStream");

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };

ZlibOutStream::ZlibOutStream(OutStream* os, int bufSize_, int compressLevel)
  : underlying(os), compressionLevel(compressLevel), newLevel(compressLevel),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0)
{
  zs = new z_stream;
  zs->zalloc    = Z_NULL;
  zs->zfree     = Z_NULL;
  zs->opaque    = Z_NULL;
  zs->next_in   = Z_NULL;
  zs->avail_in  = 0;
  if (deflateInit(zs, compressLevel) != Z_OK) {
    delete zs;
    throw Exception("ZlibOutStream: deflateInit failed");
  }
  ptr = start = new U8[bufSize];
  end = start + bufSize;
}

ZlibOutStream::~ZlibOutStream()
{
  try {
    flush();
  } catch (Exception&) {
  }
  delete [] start;
  deflateEnd(zs);
  delete zs;
}

void ZlibOutStream::setUnderlying(OutStream* os)
{
  underlying = os;
}

void ZlibOutStream::setCompressionLevel(int level)
{
  if (level < -1 || level > 9)
    level = -1;                 // Z_DEFAULT_COMPRESSION

  newLevel = level;
}

int ZlibOutStream::length()
{
  return offset + ptr - start;
}

void ZlibOutStream::flush()
{
  checkCompressionLevel();

  zs->next_in = start;
  zs->avail_in = ptr - start;

#ifdef ZLIBOUT_DEBUG
  vlog.debug("flush: avail_in %d",zs->avail_in);
#endif

  // Force out everything from the zlib encoder
  deflate(Z_SYNC_FLUSH);

  offset += ptr - start;
  ptr = start;
}

int ZlibOutStream::overrun(int itemSize, int nItems)
{
#ifdef ZLIBOUT_DEBUG
  vlog.debug("overrun");
#endif

  if (itemSize > bufSize)
    throw Exception("ZlibOutStream overrun: max itemSize exceeded");

  checkCompressionLevel();

  while (end - ptr < itemSize) {
    zs->next_in = start;
    zs->avail_in = ptr - start;

    deflate(Z_NO_FLUSH);

    // output buffer not full

    if (zs->avail_in == 0) {
      offset += ptr - start;
      ptr = start;
    } else {
      // but didn't consume all the data?  try shifting what's left to the
      // start of the buffer.
      vlog.info("z out buf not full, but in data not consumed");
      memmove(start, zs->next_in, ptr - zs->next_in);
      offset += zs->next_in - start;
      ptr -= zs->next_in - start;
    }
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}

void ZlibOutStream::deflate(int flush)
{
  int rc;

  if (!underlying)
    throw Exception("ZlibOutStream: underlying OutStream has not been set");

  if ((flush == Z_NO_FLUSH) && (zs->avail_in == 0))
    return;

  do {
    underlying->check(1);
    zs->next_out = underlying->getptr();
    zs->avail_out = underlying->getend() - underlying->getptr();

#ifdef ZLIBOUT_DEBUG
    vlog.debug("calling deflate, avail_in %d, avail_out %d",
               zs->avail_in,zs->avail_out);
#endif

    rc = ::deflate(zs, flush);
    if (rc != Z_OK) {
      // Silly zlib returns an error if you try to flush something twice
      if ((rc == Z_BUF_ERROR) && (flush != Z_NO_FLUSH))
        break;

      throw Exception("ZlibOutStream: deflate failed");
    }

#ifdef ZLIBOUT_DEBUG
    vlog.debug("after deflate: %d bytes",
               zs->next_out-underlying->getptr());
#endif

    underlying->setptr(zs->next_out);
  } while (zs->avail_out == 0);
}

void ZlibOutStream::checkCompressionLevel()
{
  int rc;

  if (newLevel != compressionLevel) {
#ifdef ZLIBOUT_DEBUG
    vlog.debug("change: avail_in %d",zs->avail_in);
#endif

    // zlib is just horribly stupid. It does an implicit flush on
    // parameter changes, but the flush it does is not one that forces
    // out all the data. And since you cannot flush things again, we
    // cannot force out our data after the parameter change. Hence we
    // need to do a more proper flush here first.
    deflate(Z_SYNC_FLUSH);

    rc = deflateParams (zs, newLevel, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK) {
      // The implicit flush can result in this error, caused by the
      // explicit flush we did above. It should be safe to ignore though
      // as the first flush should have left things in a stable state...
      if (rc != Z_BUF_ERROR)
        throw Exception("ZlibOutStream: deflateParams failed");
    }

    compressionLevel = newLevel;
  }
}
