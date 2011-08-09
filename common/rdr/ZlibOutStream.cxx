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

#include <rdr/ZlibOutStream.h>
#include <rdr/Exception.h>
#include <os/print.h>

#include <zlib.h>

using namespace rdr;

enum { DEFAULT_BUF_SIZE = 16384 };

ZlibOutStream::ZlibOutStream(OutStream* os, int bufSize_, int compressLevel)
  : underlying(os), compressionLevel(compressLevel), newLevel(compressLevel),
    bufSize(bufSize_ ? bufSize_ : DEFAULT_BUF_SIZE), offset(0),
    newBehavior(false)
{
  zs = new z_stream;
  zs->zalloc    = Z_NULL;
  zs->zfree     = Z_NULL;
  zs->opaque    = Z_NULL;
  if (deflateInit(zs, compressLevel) != Z_OK) {
    delete zs;
    throw Exception("ZlibOutStream: deflateInit failed");
  }
  ptr = start = new U8[bufSize];
  end = start + bufSize;
  const char *version = zlibVersion();
  if (strcmp(version, "1.2.3") > 0) newBehavior = true;
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
  zs->next_in = start;
  zs->avail_in = ptr - start;

//    fprintf(stderr,"zos flush: avail_in %d\n",zs->avail_in);

  if (!underlying)
    throw Exception("ZlibOutStream: underlying OutStream has not been set");

  while (zs->avail_in != 0) {

    do {
      underlying->check(1);
      zs->next_out = underlying->getptr();
      zs->avail_out = underlying->getend() - underlying->getptr();

//        fprintf(stderr,"zos flush: calling deflate, avail_in %d, avail_out %d\n",
//                zs->avail_in,zs->avail_out);
      checkCompressionLevel();
      if (zs->avail_in != 0) {
        int rc = deflate(zs, Z_SYNC_FLUSH);
        if (rc != Z_OK) throw Exception("ZlibOutStream: deflate failed");
      }

//        fprintf(stderr,"zos flush: after deflate: %d bytes\n",
//                zs->next_out-underlying->getptr());

      underlying->setptr(zs->next_out);
    } while (zs->avail_out == 0);
  }

  offset += ptr - start;
  ptr = start;
}

int ZlibOutStream::overrun(int itemSize, int nItems)
{
//    fprintf(stderr,"ZlibOutStream overrun\n");

  if (itemSize > bufSize)
    throw Exception("ZlibOutStream overrun: max itemSize exceeded");

  if (!underlying)
    throw Exception("ZlibOutStream: underlying OutStream has not been set");

  while (end - ptr < itemSize) {
    zs->next_in = start;
    zs->avail_in = ptr - start;

    do {
      underlying->check(1);
      zs->next_out = underlying->getptr();
      zs->avail_out = underlying->getend() - underlying->getptr();

//        fprintf(stderr,"zos overrun: calling deflate, avail_in %d, avail_out %d\n",
//                zs->avail_in,zs->avail_out);

     checkCompressionLevel();
     if (zs->avail_in != 0) {
       int rc = deflate(zs, 0);
       if (rc != Z_OK) throw Exception("ZlibOutStream: deflate failed");
     }

//        fprintf(stderr,"zos overrun: after deflate: %d bytes\n",
//                zs->next_out-underlying->getptr());

      underlying->setptr(zs->next_out);
    } while (zs->avail_out == 0);

    // output buffer not full

    if (zs->avail_in == 0) {
      offset += ptr - start;
      ptr = start;
    } else {
      // but didn't consume all the data?  try shifting what's left to the
      // start of the buffer.
      fprintf(stderr,"z out buf not full, but in data not consumed\n");
      memmove(start, zs->next_in, ptr - zs->next_in);
      offset += zs->next_in - start;
      ptr -= zs->next_in - start;
    }
  }

  if (itemSize * nItems > end - ptr)
    nItems = (end - ptr) / itemSize;

  return nItems;
}

void ZlibOutStream::checkCompressionLevel()
{
  if (newLevel != compressionLevel) {

    // This is a horrible hack, but after many hours of trying, I couldn't find
    // a better way to make this class work properly with both Zlib 1.2.3 and
    // 1.2.5.  1.2.3 does a Z_PARTIAL_FLUSH in the body of deflateParams() if
    // the compression level has changed, and 1.2.5 does a Z_BLOCK flush.

    if (newBehavior) {
      int rc = deflate(zs, Z_SYNC_FLUSH);
      if (rc != Z_OK) throw Exception("ZlibOutStream: deflate failed");
    }

    if (deflateParams (zs, newLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
      throw Exception("ZlibOutStream: deflateParams failed");
    }
    compressionLevel = newLevel;
  }
}
