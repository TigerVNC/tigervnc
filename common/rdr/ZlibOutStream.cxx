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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <core/LogWriter.h>

#include <rdr/ZlibOutStream.h>

#include <zlib.h>

#undef ZLIBOUT_DEBUG

static core::LogWriter vlog("ZlibOutStream");

using namespace rdr;

ZlibOutStream::ZlibOutStream(OutStream* os, int compressLevel)
  : underlying(os), compressionLevel(compressLevel), newLevel(compressLevel)
{
  zs = new z_stream;
  zs->zalloc    = nullptr;
  zs->zfree     = nullptr;
  zs->opaque    = nullptr;
  zs->next_in   = nullptr;
  zs->avail_in  = 0;
  if (deflateInit(zs, compressLevel) != Z_OK) {
    delete zs;
    throw std::runtime_error("ZlibOutStream: deflateInit failed");
  }
}

ZlibOutStream::~ZlibOutStream()
{
  try {
    flush();
  } catch (std::exception&) {
  }
  deflateEnd(zs);
  delete zs;
}

void ZlibOutStream::setUnderlying(OutStream* os)
{
  underlying = os;
  if (underlying)
    underlying->cork(corked);
}

void ZlibOutStream::setCompressionLevel(int level)
{
  if (level < -1 || level > 9)
    level = -1;                 // Z_DEFAULT_COMPRESSION

  newLevel = level;
}

void ZlibOutStream::flush()
{
  BufferedOutStream::flush();
  if (underlying != nullptr)
    underlying->flush();
}

void ZlibOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);
  if (underlying != nullptr)
    underlying->cork(enable);
}

bool ZlibOutStream::flushBuffer()
{
  checkCompressionLevel();

  zs->next_in = sentUpTo;
  zs->avail_in = ptr - sentUpTo;

#ifdef ZLIBOUT_DEBUG
  vlog.debug("Flush: avail_in %d",zs->avail_in);
#endif

  // Force out everything from the zlib encoder
  deflate(corked ? Z_NO_FLUSH : Z_SYNC_FLUSH);

  sentUpTo = zs->next_in;

  return true;
}

void ZlibOutStream::deflate(int flush)
{
  int rc;

  if (!underlying)
    throw std::runtime_error("ZlibOutStream: Underlying OutStream has not been set");

  if ((flush == Z_NO_FLUSH) && (zs->avail_in == 0))
    return;

  do {
    size_t chunk;
    zs->next_out = underlying->getptr(1);
    zs->avail_out = chunk = underlying->avail();

#ifdef ZLIBOUT_DEBUG
    vlog.debug("Calling deflate, avail_in %d, avail_out %d",
               zs->avail_in,zs->avail_out);
#endif

    rc = ::deflate(zs, flush);
    if (rc < 0) {
      // Silly zlib returns an error if you try to flush something twice
      if ((rc == Z_BUF_ERROR) && (flush != Z_NO_FLUSH))
        break;

      throw std::runtime_error("ZlibOutStream: deflate failed");
    }

#ifdef ZLIBOUT_DEBUG
    vlog.debug("After deflate: %d bytes",
               zs->next_out-underlying->getptr());
#endif

    underlying->setptr(chunk - zs->avail_out);
  } while (zs->avail_out == 0);
}

void ZlibOutStream::checkCompressionLevel()
{
  int rc;

  if (newLevel != compressionLevel) {
#ifdef ZLIBOUT_DEBUG
    vlog.debug("Change: avail_in %d",zs->avail_in);
#endif

    // zlib is just horribly stupid. It does an implicit flush on
    // parameter changes, but the flush it does is not one that forces
    // out all the data. And since you cannot flush things again, we
    // cannot force out our data after the parameter change. Hence we
    // need to do a more proper flush here first.
    deflate(Z_SYNC_FLUSH);

    rc = deflateParams (zs, newLevel, Z_DEFAULT_STRATEGY);
    if (rc < 0) {
      // The implicit flush can result in this error, caused by the
      // explicit flush we did above. It should be safe to ignore though
      // as the first flush should have left things in a stable state...
      if (rc != Z_BUF_ERROR)
        throw std::runtime_error("ZlibOutStream: deflateParams failed");
    }

    compressionLevel = newLevel;
  }
}
